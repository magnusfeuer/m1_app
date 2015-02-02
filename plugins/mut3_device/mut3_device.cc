//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "mut3_device.hh"
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include <linux/serial.h>
XOBJECT_TYPE_BOOTSTRAP(CMUT3Device);
unsigned long mut_ts;

unsigned long time_stamp(void)
{
    struct timeval tm;
    static unsigned long long first_ts = 0LL;
    unsigned long long ts;

    gettimeofday(&tm, 0);
    ts =tm.tv_sec*1000000LL + tm.tv_usec;

    if (first_ts == 0LL) 
	first_ts = ts;

    return (unsigned long) (ts-first_ts)/1000LL; // msec
}

void dump(char *hdr, uchar *data, int len) 
{
    int i;
    printf("%s: ", hdr);
    for( i = 0; i < len; ++i) 
	printf("0x%.2X ", data[i] & 0xFF);
	
    putchar('\n');
}

void dumptime(char *prompt)
{
    struct timeval tid;
    gettimeofday(&tid, 0);
    DBGFMT("%.40s %d.%.6d", prompt, tid.tv_sec, tid.tv_usec);     
}


CMUT3Device::CMUT3Device(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDescriptor(-1),
    mCurrentPID(0),
    mFaultCount(0),
    mPort(this),
    mState(this),
    mRevents(this),
    mSource(NULL),
    mLastOpenAttempt(0),
    mPortReopenTimer(NULL),
    mPortReopenTimeout(this),
    mReadTimeoutTimer(NULL),
    mReadTimeout(this),
    mPortSetupTimeoutTimer(NULL),
    mPortSetupTimeout(this)
{ 
    int i;

    mPIDs[0] =  new CPID(aExec, "rpm",      this, (uchar) 0x21, false, 0.0, 31.25, 0.0, 0, 8000.0 );
    mPIDs[1] =  new CPID(aExec, "timing",   this, (uchar) 0x06, false, 0.0, 1.0, -20.0, -20.0, 235.0);
    mPIDs[2] =  new CPID(aExec, "spd",      this, (uchar) 0x2F, false, 0.0, 2.0, 0.0, 0.0, 512.0 );
    mPIDs[3] =  new CPID(aExec, "lam_1",    this, (uchar) 0x13, false, 0.0, 0.0195, 0.0, 0.0, 2.0 );
    mPIDs[4] =  new CPID(aExec, "lam_2",    this, (uchar) 0x3C, false, 0.0, 0.0195, 0.0, 0.0, 2.0 ); // Not used by mut_engine.m1
    mPIDs[5] =  new CPID(aExec, "maf",      this, (uchar) 0x1A, false, 0.0, 0.824, 0.0, 0.0, 210.0); // Raw value, must be calibrated.
    mPIDs[6] = new CPID(aExec, "tps",      this, (uchar) 0x17, false, 0.0, 0.39215, 0.0, 0.0, 100.0 );  

    mPIDs[7] = new CPID(aExec, "ect",      this, (uchar) 0x07, false, 0.0, 0.0, 0.0, -47.0, 152.0 ); // C

    //    mPIDs[8] = new CPID(aExec, "map",      this, (uchar) 0x38, false, 0.0, 1.25, 0.0, 0.0, 318.0 );  // NEEDS CALIBRATION
    mPIDs[8] = new CPID(aExec, "map",      this, (uchar) 0x38, false, 0.0, 5.0, 0.0, 0.0, 1235.0 );  // NEEDS CALIBRATION
    mPIDs[9] = new CPID(aExec, "act",      this, (uchar) 0x3A, false, 0.0, 0.0, 0.0, -55.0, 200.0 );

    mPIDs[10] = new CPID(aExec, "bap",      this, (uchar) 0x15, false, 0.0, 4.486, 0.0, 0.0, 1240 ); // FUTURE
    mPIDs[11] = new CPID(aExec, "vbat",     this, (uchar) 0x14, false, 0.0, 0.0733, 0.0, 0.1, 18.7);
    mPIDs[12] = new CPID(aExec, "fpw",      this, (uchar) 0x29, false, 0.0, 0.256, 0.0, 0.0, 20.0);
    mPIDs[13] = new CPID(aExec, "wgdc",      this, (uchar) 0x14, false, 0.0, 0.39215, 0.0, 0.0, 100.0);

    eventPut(aExec, XINDEX(CMUT3Device, revents), &mRevents);  // File revents trigger event.

    eventPut(aExec, XINDEX(CMUT3Device, port), &mPort);
    mState.putValue(aExec, UNDEFINED);
    eventPut(aExec, XINDEX(CMUT3Device, state), &mState);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);
    connect(XINDEX(CMUT3Device,revents), mSource, XINDEX(CFileSource,revents));

    //
    // Create port reopen timer.
    //
    //    mPortReopenTimer = m1New(CTimeout, aExec);

    mPortReopenTimer = (CTimeout *) CTimeout::CTimeoutType::singleton()->produce(aExec, NULL, NULL).o;
    m1Retain(CTimer, mPortReopenTimer);
    aExec->addComponent(mPortReopenTimer);

    //
    // Setup port reopen to trigger in 2.5 sec.
    // We will use this during startup to wait 2.5 seconds until we open
    // the port in order to let the rest of the system initialize correctly.
    //
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(7.0));

    eventPut(aExec, XINDEX(CMUT3Device, port_reopen_timeout), &mPortReopenTimeout);
    connect(XINDEX(CMUT3Device, port_reopen_timeout), mPortReopenTimer, XINDEX(CTimeout, timeout));

    //
    // Create read timeout timer.
    //
    mReadTimeoutTimer = m1New(CTimeout, aExec);
    m1Retain(CTimeout, mReadTimeoutTimer);
    aExec->addComponent(mReadTimeoutTimer);

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(0.5));

    eventPut(aExec, XINDEX(CMUT3Device, read_timeout), &mReadTimeout);
    connect(XINDEX(CMUT3Device, read_timeout), mReadTimeoutTimer, XINDEX(CTimeout, timeout));

    //
    // Create a port setup timeout timer to send a break for 1.8 sec.
    //
    mPortSetupTimeoutTimer = m1New(CTimeout, aExec);
    m1Retain(CTimeout, mPortSetupTimeoutTimer);
    aExec->addComponent(mPortSetupTimeoutTimer);

    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(1.7));
    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false), TRIGGER_YES);

    mPortSetupTimeout.putValue(aExec, false);
    eventPut(aExec, XINDEX(CMUT3Device, port_setup_timeout), &mPortSetupTimeout);
    connect(XINDEX(CMUT3Device, port_setup_timeout), mPortSetupTimeoutTimer, XINDEX(CTimeout, timeout));

    i = MUT3_PID_COUNT;
    while(i--) {
	mPIDs[i]->mValue.putValue(aExec, mPIDs[i]->mMinValue.value());
	mPIDs[i]->mSupported.putValue(aExec, true);
	mPIDs[i]->mUsage.putValue(aExec, 0);
    }

    eventPut(aExec, XINDEX(CMUT3Device, rpm), &(mPIDs[0]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, timing), &(mPIDs[1]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, spd), &(mPIDs[2]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, lam_1), &(mPIDs[3]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, lam_2), &(mPIDs[4]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, maf), &(mPIDs[5]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, tps), &(mPIDs[6]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, ect), &(mPIDs[7]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, map), &(mPIDs[8]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, act), &(mPIDs[9]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, bap), &(mPIDs[10]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, vbat), &(mPIDs[11]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, fpw), &(mPIDs[12]->mValue));
    eventPut(aExec, XINDEX(CMUT3Device, wgdc), &(mPIDs[13]->mValue));

    eventPut(aExec, XINDEX(CMUT3Device, rpmSupported), &(mPIDs[0]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, timingSupported), &(mPIDs[1]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, spdSupported), &(mPIDs[2]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, lam_1Supported), &(mPIDs[3]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, lam_2Supported), &(mPIDs[4]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, mafSupported), &(mPIDs[5]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, tpsSupported), &(mPIDs[6]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, ectSupported), &(mPIDs[7]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, mapSupported), &(mPIDs[8]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, actSupported), &(mPIDs[9]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, bapSupported), &(mPIDs[10]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, vbatSupported), &(mPIDs[11]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, fpwSupported), &(mPIDs[12]->mSupported));
    eventPut(aExec, XINDEX(CMUT3Device, wgdcSupported), &(mPIDs[13]->mSupported));


    eventPut(aExec, XINDEX(CMUT3Device, rpmUsage), &(mPIDs[0]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, timingUsage), &(mPIDs[1]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, spdUsage), &(mPIDs[2]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, lam_1Usage), &(mPIDs[3]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, lam_2Usage), &(mPIDs[4]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, mafUsage), &(mPIDs[5]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, tpsUsage), &(mPIDs[6]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, ectUsage), &(mPIDs[7]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, mapUsage), &(mPIDs[8]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, actUsage), &(mPIDs[9]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, bapUsage), &(mPIDs[10]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, vbatUsage), &(mPIDs[11]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, fpwUsage), &(mPIDs[12]->mUsage));
    eventPut(aExec, XINDEX(CMUT3Device, wgdcUsage), &(mPIDs[13]->mUsage));

    eventPut(aExec, XINDEX(CMUT3Device, rpmMin), &(mPIDs[0]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, timingMin), &(mPIDs[1]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, spdMin), &(mPIDs[2]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, lam_1Min), &(mPIDs[3]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, lam_2Min), &(mPIDs[4]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, mafMin), &(mPIDs[5]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, tpsMin), &(mPIDs[6]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, ectMin), &(mPIDs[7]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, mapMin), &(mPIDs[8]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, actMin), &(mPIDs[9]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, bapMin), &(mPIDs[10]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, vbatMin), &(mPIDs[11]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, fpwMin), &(mPIDs[12]->mMinValue));
    eventPut(aExec, XINDEX(CMUT3Device, wgdcMin), &(mPIDs[13]->mMinValue));

    eventPut(aExec, XINDEX(CMUT3Device, rpmMax), &(mPIDs[0]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, timingMax), &(mPIDs[1]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, spdMax), &(mPIDs[2]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, lam_1Max), &(mPIDs[3]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, lam_2Max), &(mPIDs[4]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, mafMax), &(mPIDs[5]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, tpsMax), &(mPIDs[6]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, ectMax), &(mPIDs[7]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, mapMax), &(mPIDs[8]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, actMax), &(mPIDs[9]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, bapMax), &(mPIDs[10]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, vbatMax), &(mPIDs[11]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, fpwMax), &(mPIDs[12]->mMaxValue));
    eventPut(aExec, XINDEX(CMUT3Device, wgdcMax), &(mPIDs[13]->mMaxValue));

    setFlags(ExecuteOnEventUpdate);
}

    
CMUT3Device::~CMUT3Device(void)
{
    DBGFMT_CLS("CMUT3Device::~CMUT3Device(): Called");

    if (mDescriptor > -1) 
	close(mDescriptor);

    disconnect(XINDEX(CMUT3Device,revents));
    m1Release(CFileSource, mSource);

    m1Release(CTimeout, mReadTimeoutTimer);
    m1Release(CTimeout, mPortReopenTimer);
    m1Release(CTimeout, mPortSetupTimeoutTimer);
}


bool CMUT3Device::increaseFaultCounter(void) 
{

    mFaultCount++;
    if (mFaultCount == 10) {
	DBGFMT_CLS("CMUT3Device::increaseFaultCounter(): Ten failed PID requests. Resetting port.");
	return false;
    }
    return true;
}

bool CMUT3Device::resetFaultCounter(void) 
{
    mFaultCount = 0;
}

bool CMUT3Device::processStream(CExecutor* aExec) 
{
    uchar buf[128];
    float val;
    CPID *pid;
    uchar *current = buf;
    int len = 0; // Initial count.
    int res;
    struct pollfd fd;
    
    fd.events = POLLIN;
    fd.revents = 0;
    fd.fd = mDescriptor;

    if (mDescriptor == -1)
	return false;

    //
    // Data should be readily available since we triggered on poll, but that is not always the case.
    //
    if (m1Poll(&fd, 1, 0) <= 0) 
	return true;

    len = read(mDescriptor, buf, 2);
  
    if (len == -1) {
	DBGFMT_CLS("CMUT3Device::processStream(): Could not read data [%s]", strerror(errno));
	closePort(aExec);
	return false;
    }
//      printf("CMUT3Device::processStream(): Got [");
//      for (int i = 0; i < len; ++i) 
//    	printf("[0x%.2X]", buf[i] & 0xFF);
//      puts("]");
			     
    if (len < 2) {
	DBGFMT_CLS("CMUT3Device::processStream(): Got less than 2 bytes: %d.", len);
	if (!increaseFaultCounter())  {
	    closePort(aExec);
	    return false;
	}
	if (advancePID())
	    requestPID(aExec, mCurrentPID);

	return false;
    }

    pid = mPIDs[mCurrentPID];
    if (buf[0] != pid->mPID) {
	DBGFMT_CLS("CMUT3Device::processStream(): Expected response pid [0x%.2X] - Got [0x%.2X]",  
		   pid->mPID & 0xFF,
		   buf[0] & 0xFF);

	if (!increaseFaultCounter())  {
	    closePort(aExec);
	    return false;
	}

	if (advancePID())
	    requestPID(aExec, mCurrentPID);

	return true;
    }
	
    resetFaultCounter();
    if (pid->mInvert)
	val = float(0xFF - buf[1]);
    else
	val = float(buf[1]);

    // Special calc case for ect, non linear thermistors.
    switch(pid->mPID) {
    case 0x07: // ECT non linear.	
	pid->mValue.putValue(aExec, (-0.00000003166*powf(val, 5)+0.00001425*powf(val,4)-0.002490*powf(val,3)+0.2143*powf(val,2)-10.279*val+361.01 - 32) * 0.555556 ) ;
	//                                     -0.0003893*      x^3      +0.08056*       x^2-     6.5226*   x+    315.73
	//	pid->mValue.putValue(aExec, (-0.0003893 * powf(val,3) + 0.08056 * powf(val, 2) - 6.5226 * val + 315.73 - 32) * 0.555556);
	DBGFMT_CLS("CMUT3Device::processStream(%f): [%s] = [%f]",
		   float(buf[1]),
		   pid->mName, 
		   pid->mValue.value());
	break;
	
    case 0x3A: // IAT non linear
	pid->mValue.putValue(aExec, (-0.00000003166*powf(val, 5)+0.00001425*powf(val,4)-0.002490*powf(val,3)+0.2143*powf(val,2)-10.279*val+361.01 - 32) * 0.555556 ) ;
	DBGFMT_CLS("CMUT3Device::processStream(%f): [%s] = [%f]",
		   float(buf[1]),
		   pid->mName, 
		   pid->mValue.value());
	break;
    
    default:
	pid->mValue.putValue(aExec, pid->mPreAdd + val * pid->mMultiplier + pid->mPostAdd);
	
	DBGFMT_CLS("CMUT3Device::processStream(%f): [%s] = [%f] + [%f] * [%f] + [%f] = [%f]",
		   float(buf[1]),
		   pid->mName, 
		   pid->mPreAdd, 
		   float(val),
		   pid->mMultiplier, 
		   pid->mPostAdd,
		   pid->mValue.value());
	break;
    }

    if (advancePID())
	requestPID(aExec, mCurrentPID);

    return true;
}



bool CMUT3Device::setBaudrate(int aBaudrate) 
{
    struct serial_struct serinfo;
    struct termios       term;
    
    if (mDescriptor == -1)
	return false;

    errno = 0;
    if (ioctl(mDescriptor, TIOCGSERIAL, &serinfo) < 0) {
	DBGFMT_CLS("Cannot TIOCGSERIAL: %s");
	return false;
    }

    serinfo.custom_divisor = serinfo.baud_base / aBaudrate; 
    serinfo.flags = (serinfo.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;

    if (ioctl(mDescriptor, TIOCSSERIAL, &serinfo) < 0){
	DBGFMT_CLS("Cannot TIOCSSERIAL: %s");
	return false;
    }

    if (tcgetattr(mDescriptor, &term) < 0){
	DBGFMT_CLS("Cannot tcgetattr: %s");
	return false;
    }

    // Enable custom baud speed.
    cfsetospeed(&term, B38400);
    cfsetispeed(&term, B0);

    if (tcsetattr(mDescriptor, TCSANOW, &term) < 0){
	DBGFMT_CLS("Cannot tcsetattr: %s");
	return false;
    }
    return true;
}

bool CMUT3Device::setSerialState(bool aBreak, bool aRTS)
{
  int flags = 0; 
  int setflags = 0, clearflags = 0;
  
  if (aBreak)
    setflags   |= TIOCM_DTR;
  else
    clearflags |= TIOCM_DTR;

  if (aRTS)
    setflags   |= TIOCM_RTS;
  else
    clearflags |= TIOCM_RTS;

  errno = 0;
  if (ioctl(mDescriptor, TIOCMGET, &flags) < 0){
      //    fprintf(stderr, "open: Ioctl TIOCMGET failed %s\n", strerror(errno));
      flags = 0x166; // Bug workaround for buggy FTDI driver in older kernels.
  } 

  //  printf("Flags[0x%X]\n", flags);

  flags |= setflags;
  flags &= ~clearflags;

  if (ioctl(mDescriptor, TIOCMSET, &flags) < 0){
    DBGFMT_CLS("open: Ioctl TIOCMSET failed %s", strerror(errno));
    return false;
  }
  return true;
}





bool CMUT3Device::openPort(CExecutor* aExec, TimeStamp aTimeStamp)
{
    uchar res[100];
    struct termios options;


    mState.putValue(aExec, RUNNING);
    // Rst timer.
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false), TRIGGER_YES);

    if (mDescriptor != -1)
	close(mDescriptor);

    printf("CMUT3Device::openPort(): (Re)opening device [%s] for MUT-III communication.\n", mPort.value().c_str());
    if ((mDescriptor = open(mPort.value().c_str(), O_RDWR)) < 0) {
	printf("CMUT3Device:openPort(): Could not open [%s]: %s\n", 
		   mPort.value().c_str(), strerror(errno));
	closePort(aExec);
	return false;
    }

    tcgetattr(mDescriptor, &options);
    cfmakeraw(&options);
    options.c_cflag = CS8 | CLOCAL | CREAD | B38400;
    options.c_iflag = IGNBRK | IGNPAR; // Ignore break, parity and map cr->nl
    options.c_cc[VTIME] = 3; // 100 msec tout
    options.c_cc[VMIN] = 2;
    tcsetattr(mDescriptor, TCSANOW, &options);

    if (!setBaudrate(MUT3_BAUDRATE)) {
	DBGFMT_CLS("CMUT3Device:openPort(): Could not set baudrate on [%s]: %s", 
		   mPort.value().c_str(), strerror(errno));
	closePort(aExec);
	return false;
    }

    write(mDescriptor, "", 0); // Needed to get K-line resistance to 1.8kohm.
    setSerialState(true, true);


//     usleep(1800000);
//     setSerialState(false, true);
//     usleep(200000);
//     setSerialState(true, false);
//     usleep(200000);
//     setSerialState(false, false);
    mut_ts = m1_TimeStampToTime(aExec->timeStamp());

    DBGFMT_CLS("Start: %lu", mut_ts);

    //
    // Use the read timeout timer when connecting to trigger in 1800 msec 
    // to simulate a 5 bauf init.
    //
    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);
    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true), TRIGGER_YES);
    //    mState.putValue(aExec, RUNNING);


//     usleep(1800000);
//     printf("ts complete at [%lu]\n", m1_TimeStampToTime(aExec->timeStamp()) - mut_ts);
//     return completePortSetup(aExec);
}

//
// Calle 1800 msec after openPort to complete
// MUT III init.
//
bool CMUT3Device::completePortSetup(CExecutor *aExec) 
{
    char buf[128];
    int len =0;
    int i;
    int cnt;
    struct pollfd fd;
    
    fd.events = POLLIN;
    fd.revents = 0;
    fd.fd = mDescriptor;

    if (mDescriptor == -1)
	return false;


    DBGFMT_CLS("CMUT3Device::completePortSetup(): Cleared break at [%lu].",m1_TimeStampToTime(aExec->timeStamp()) - mut_ts);
    setSerialState(false, true);
    usleep(200000);
    setSerialState(true, false);
    usleep(200000);
    setSerialState(false, false);

    //    mState.putValue(aExec, RUNNING);
    if (m1Poll(&fd, 1, 200) == 1) 
	len = read(mDescriptor, buf, 128);
    else 
	DBGFMT_CLS("CMUT3Device::completePortSetup(): No init response");

//     printf("Got [%d] bytes: ", len);

//     for( i = 0; i < len; ++i) 
// 	printf("0x%.2X ", buf[i] & 0xFF);
	
//     putchar('\n');
   

    //      { 
    //         unsigned char buf[2]; 
    //          unsigned char init[] = { 
    //    	  MUTINIT0, 
    //    	  MUTINIT1, 
    //    	  MUTINIT2,
    //    	  MUTINIT3,
    //          }; 

    //          for(i=0; i<4; i++){ 
    //    	  write(conn->fd, &init[i], 1); 
    //    	  get_char(conn->fd); 
    //          } 
    //        } 

//     read(mDescriptor, buf, 128);

    //
    // Setup the next pid to request.
    //
    mCurrentPID = 0;
    resetFaultCounter();
    advancePID();
    requestPID(aExec, mCurrentPID);
  
//      cnt = 10;
//      while(cnt--) {
//  	buf[0] = 0x07;
//  	write(mDescriptor, buf, 1);
//  	buf[0] = 0;
//  	buf[1] = 0;
//  	len = read(mDescriptor,buf, 2);
//  	printf("len[%d]: ", len);


//  	for( i = 0; i < len; ++i) 
//  	    printf("0x%.2X ", buf[i] & 0xFF);
	
//  	putchar('\n');
//      }

    mSource->setDescriptor(aExec, mDescriptor, POLLIN);
    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(1.8), TRIGGER_YES);
    DBGFMT_CLS("CMUT3Device::completePortSetup(): Done.");
    return true;
}

bool CMUT3Device::requestPID(CExecutor *aExec, int aIndex)
{
    if (aIndex < 0 || 
	aIndex>= MUT3_PID_COUNT ||
	mDescriptor == -1) 
	return true;

    DBGFMT_CLS("CMUT3Device::requestPID(): Requesting. PID[%s][0x%.2X]", mPIDs[aIndex]->mName, mPIDs[aIndex]->mPID);

    if (write(mDescriptor, &(mPIDs[aIndex]->mPID), 1) != 1) {
	closePort(aExec);
	return false;
    }

    //
    // Reset the reader timeout.
    //
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true), TRIGGER_YES);
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);
    return true;
}

// 
// Advance to the next PID to be queried.
//
bool CMUT3Device::advancePID(void)
{
    int start_ind;

    // Advance current pid by one (with wrap) and remember 
    // where we started.
    start_ind = mCurrentPID;
    
    mCurrentPID = (mCurrentPID + 1) % MUT3_PID_COUNT;

    while(mCurrentPID != start_ind) {
	if (mPIDs[mCurrentPID]->mUsage.value() > 0) 
	    return true;
	
	mCurrentPID = (mCurrentPID + 1) % MUT3_PID_COUNT;
    }

    if (mPIDs[mCurrentPID]->mUsage.value() > 0) 
	return true;

    mCurrentPID = 0; // We must always query ECU, even when dormant, or else we will get coms timeout.
    return true;
}


void CMUT3Device::closePort(CExecutor *aExec)
{
    DBGFMT_CLS("MUT3: Resetting port.");
    mState.putValue(aExec, UNDEFINED);

    if (mDescriptor != -1) {
	close(mDescriptor);
	mDescriptor = -1;
	mSource->setDescriptor(aExec, -1);
    }

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false), TRIGGER_YES);
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);
    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false), TRIGGER_YES);
    mPortSetupTimeoutTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true), TRIGGER_YES);
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);
}

void CMUT3Device::start(CExecutor *aExec) 
{
    mState.putValue(aExec, RUNNING);
}


void CMUT3Device::execute(CExecutor* aExec)
{
    TimeStamp tTimeStamp = aExec->cycleTime();

    //
    // Port is updated. Close old descriptor and open new one.
    //
    if (mPort.value() != "" && (mPortReopenTimeout.updated() && mPortReopenTimeout.value() == true)) {
	DBGFMT_CLS("port field updated to [%s]. Will open.", mPort.value().c_str());
	openPort(aExec, tTimeStamp);
	return;
    }

    if (mDescriptor == -1)  {
	DBGFMT_CLS("CMUT3Device::execute(): mDescriptor==-1");
	return;
    }
    
    //
    // Check if we have a port setup timeout indicating that we waited 1.8 seconds
    // after setting the break condition. If so, complete port setup
    //
    if (mPortSetupTimeout.updated() && mPortSetupTimeout.value() == true) {    
	if (!completePortSetup(aExec))  {
	    DBGFMT_CLS("Could not complete port setup.");
	    closePort(aExec);
	}
	return;
    }	

    if (mRevents.updated() && mState.value() == RUNNING) 
	processStream(aExec);

    //
    // Read timeout while waiting for a response from a pid request.
    //
    if (mReadTimeout.updated() && mReadTimeout.value() == true) {
	DBGFMT_CLS("Lost communication. Trying to reopen port.");
	closePort(aExec);
	return;
    }

    return;
}

