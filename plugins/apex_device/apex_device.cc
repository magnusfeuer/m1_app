//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "apex_device.hh"
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

XOBJECT_TYPE_BOOTSTRAP(CApexDevice);

void dumptime(char *prompt)
{
    struct timeval tid;
    gettimeofday(&tid, 0);
    DBGFMT("%.40s %d.%.6d", prompt, tid.tv_sec, tid.tv_usec);     
}


CApexDevice::CApexDevice(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDescriptor(-1),
    mPort(this),
    mProtocol(this),
    mState(this),
    mRevents(this),
    mIsoGapTiming(this),
    mIsoGapTimingPending(false),
    mSource(NULL),
    mLastOpenAttempt(0),
    mPortReopenTimer(NULL),
    mPortReopenTimeout(this),
    mReadTimeoutTimer(NULL),
    mReadTimeout(this),
    mLatency(this),
    mReadFailCount(0)
{ 
    int i = sizeof(mPIDs)/sizeof(mPIDs[0]);
    
    mProtocol.putValue(aExec, UNKNOWN);

    mPIDs[0] =  new CPID(aExec, 2, "s_trim_1", this, (unsigned char) 0x06, 1, -128, 0.7812, 0, -100, 100);
    mPIDs[1] =  new CPID(aExec, 3, "l_trim_1", this , (unsigned char) 0x07, 1, -128, 0.7812, 0, -100, 100 );
    mPIDs[2] =  new CPID(aExec, 2, "s_trim_2", this , (unsigned char) 0x08, 1, -128, 0.7812, 0, -100, 100 );
    mPIDs[3] =  new CPID(aExec, 3, "l_trim_2", this , (unsigned char) 0x09, 1, -128, 0.7812, 0, -100, 100 );
    mPIDs[4] =  new CPID(aExec, 0, "timing", this , (unsigned char) 0x0E, 1, 0, 0.5, -65, -64, 64 );
    mPIDs[5] =  new CPID(aExec, 0, "rpm", this , (unsigned char) 0x0C, 2, 0, 0.25, 0, 0, 16383 );
    mPIDs[6] =  new CPID(aExec, 1, "spd", this , (unsigned char) 0x0D, 1, 0, 1.0,  0, 0, 255 );
    mPIDs[7] =  new CPID(aExec, 0, "lam_1", this , (unsigned char) 0x24, 4, 0, 0.0000305, 0, 0.0, 2.0 );
    mPIDs[8] =  new CPID(aExec, 0, "lam_2", this , (unsigned char) 0x25, 4, 0, 0.0000305, 0, 0.0, 2.0 );
    mPIDs[9] =  new CPID(aExec, 0, "lam_3", this , (unsigned char) 0x26, 4, 0, 0.0000305, 0, 0.0, 2.0 );
    mPIDs[10] =  new CPID(aExec, 0, "lam_4", this , (unsigned char) 0x27, 4, 0, 0.0000305, 0, 0.0, 2.0 );
    mPIDs[11] =  new CPID(aExec, 0, "maf", this , (unsigned char) 0x10, 2, 0, 0.01, 0, 0, 655 ); // gr/sec.
    mPIDs[12] = new CPID(aExec, 1, "load", this , (unsigned char) 0x04, 1, 0, 0.39215686, 0, 0, 100 );
    mPIDs[13] = new CPID(aExec, 0, "tps", this , (unsigned char) 0x11, 1, 0, 0.39215686, 0, 0, 100 );  
    mPIDs[14] = new CPID(aExec, 3, "ect", this , (unsigned char) 0x05, 1, 0, 1.0, -40, -40, 215 );
    mPIDs[15] = new CPID(aExec, 3, "fp", this , (unsigned char) 0x0A, 1, 0, 30.0, 0, -0, 7650);
    mPIDs[16] = new CPID(aExec, 0, "map", this , (unsigned char) 0x0B, 1, 0, 10.0, 0, 0, 2550 );
    mPIDs[17] = new CPID(aExec, 3, "act", this , (unsigned char) 0x0F, 1, 0, 1.0, -40, -40, 215 );
    mPIDs[18] = new CPID(aExec, 3, "fuel", this , (unsigned char) 0x2F, 1, 0, 0.39215686, 0, 0, 100 );
    mPIDs[19] = new CPID(aExec, 2, "bap", this , (unsigned char) 0x33, 1, 0, 10.0, 0, 0, 2550 );
    mPIDs[20] = new CPID(aExec, 3, "vbat", this , (unsigned char) 0x42, 2, 0, 0.001, 0, 0, 65.5);

    eventPut(aExec, XINDEX(CApexDevice, protocol), &mProtocol);  // File revents trigger event.
    eventPut(aExec, XINDEX(CApexDevice, revents), &mRevents);  // File revents trigger event.
    eventPut(aExec, XINDEX(CApexDevice, iso_gap_time), &mIsoGapTiming);  // ISO gap time parameter.

    mLatency.putValue(aExec, 0.0);
    eventPut(aExec, XINDEX(CApexDevice, latency), &mLatency);  // ECU response time.

    eventPut(aExec, XINDEX(CApexDevice, port), &mPort);
    mState.putValue(aExec, APEX_UNDEFINED);
    eventPut(aExec, XINDEX(CApexDevice, state), &mState);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);
    connect(XINDEX(CApexDevice,revents), mSource, XINDEX(CFileSource,revents));

    //
    // Create port reopen timer.
    //
    mPortReopenTimer = m1New(CTimeout, aExec);
    m1Retain(CTimer, mPortReopenTimer);
    aExec->addComponent(mPortReopenTimer);
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(1.0));

    eventPut(aExec, XINDEX(CApexDevice, port_reopen_timeout), &mPortReopenTimeout);
    connect(XINDEX(CApexDevice, port_reopen_timeout), mPortReopenTimer, XINDEX(CTimeout, timeout));

    //
    // Create read timeout timer.
    //
    mReadTimeoutTimer = m1New(CTimeout, aExec);
    m1Retain(CTimeout, mReadTimeoutTimer);
    aExec->addComponent(mReadTimeoutTimer);

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(0.5));

    eventPut(aExec, XINDEX(CApexDevice, read_timeout), &mReadTimeout);
    connect(XINDEX(CApexDevice, read_timeout), mReadTimeoutTimer, XINDEX(CTimeout, timeout));

    while(i--) {
	mPIDs[i]->mValue.putValue(aExec, mPIDs[i]->mMinValue.value());
	mPIDs[i]->mUsage.putValue(aExec, 0);
    }

    eventPut(aExec, XINDEX(CApexDevice, s_trim_1), &(mPIDs[0]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_1), &(mPIDs[1]->mValue)); 
    eventPut(aExec, XINDEX(CApexDevice, s_trim_2), &(mPIDs[2]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_2), &(mPIDs[3]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, timing), &(mPIDs[4]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, rpm), &(mPIDs[5]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, spd), &(mPIDs[6]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_1), &(mPIDs[7]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_2), &(mPIDs[8]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_3), &(mPIDs[9]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_4), &(mPIDs[10]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, maf), &(mPIDs[11]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, load), &(mPIDs[12]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, tps), &(mPIDs[13]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, ect), &(mPIDs[14]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, fp), &(mPIDs[15]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, map), &(mPIDs[16]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, act), &(mPIDs[17]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, fuel), &(mPIDs[18]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, bap), &(mPIDs[19]->mValue));
    eventPut(aExec, XINDEX(CApexDevice, vbat), &(mPIDs[20]->mValue));


    eventPut(aExec, XINDEX(CApexDevice, s_trim_1Supported), &(mPIDs[0]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_1Supported), &(mPIDs[1]->mSupported)); 
    eventPut(aExec, XINDEX(CApexDevice, s_trim_2Supported), &(mPIDs[2]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_2Supported), &(mPIDs[3]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, timingSupported), &(mPIDs[4]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, rpmSupported), &(mPIDs[5]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, spdSupported), &(mPIDs[6]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, lam_1Supported), &(mPIDs[7]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, lam_2Supported), &(mPIDs[8]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, lam_3Supported), &(mPIDs[9]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, lam_4Supported), &(mPIDs[10]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, mafSupported), &(mPIDs[11]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, loadSupported), &(mPIDs[12]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, tpsSupported), &(mPIDs[13]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, ectSupported), &(mPIDs[14]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, fpSupported), &(mPIDs[15]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, mapSupported), &(mPIDs[16]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, actSupported), &(mPIDs[17]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, fuelSupported), &(mPIDs[18]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, bapSupported), &(mPIDs[19]->mSupported));
    eventPut(aExec, XINDEX(CApexDevice, vbatSupported), &(mPIDs[20]->mSupported));


    eventPut(aExec, XINDEX(CApexDevice, s_trim_1Usage), &(mPIDs[0]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_1Usage), &(mPIDs[1]->mUsage)); 
    eventPut(aExec, XINDEX(CApexDevice, s_trim_2Usage), &(mPIDs[2]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_2Usage), &(mPIDs[3]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, timingUsage), &(mPIDs[4]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, rpmUsage), &(mPIDs[5]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, spdUsage), &(mPIDs[6]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, lam_1Usage), &(mPIDs[7]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, lam_2Usage), &(mPIDs[8]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, lam_3Usage), &(mPIDs[9]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, lam_4Usage), &(mPIDs[10]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, mafUsage), &(mPIDs[11]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, loadUsage), &(mPIDs[12]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, tpsUsage), &(mPIDs[13]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, ectUsage), &(mPIDs[14]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, fpUsage), &(mPIDs[15]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, mapUsage), &(mPIDs[16]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, actUsage), &(mPIDs[17]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, fuelUsage), &(mPIDs[18]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, bapUsage), &(mPIDs[19]->mUsage));
    eventPut(aExec, XINDEX(CApexDevice, vbatUsage), &(mPIDs[20]->mUsage));

    eventPut(aExec, XINDEX(CApexDevice, s_trim_1Min), &(mPIDs[0]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_1Min), &(mPIDs[1]->mMinValue)); 
    eventPut(aExec, XINDEX(CApexDevice, s_trim_2Min), &(mPIDs[2]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_2Min), &(mPIDs[3]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, timingMin), &(mPIDs[4]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, rpmMin), &(mPIDs[5]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, spdMin), &(mPIDs[6]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_1Min), &(mPIDs[7]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_2Min), &(mPIDs[8]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_3Min), &(mPIDs[9]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_4Min), &(mPIDs[10]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, mafMin), &(mPIDs[11]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, loadMin), &(mPIDs[12]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, tpsMin), &(mPIDs[13]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, ectMin), &(mPIDs[14]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, fpMin), &(mPIDs[15]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, mapMin), &(mPIDs[16]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, actMin), &(mPIDs[17]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, fuelMin), &(mPIDs[18]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, bapMin), &(mPIDs[19]->mMinValue));
    eventPut(aExec, XINDEX(CApexDevice, vbatMin), &(mPIDs[20]->mMinValue));


    eventPut(aExec, XINDEX(CApexDevice, s_trim_1Max), &(mPIDs[0]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_1Max), &(mPIDs[1]->mMaxValue)); 
    eventPut(aExec, XINDEX(CApexDevice, s_trim_2Max), &(mPIDs[2]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, l_trim_2Max), &(mPIDs[3]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, timingMax), &(mPIDs[4]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, rpmMax), &(mPIDs[5]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, spdMax), &(mPIDs[6]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_1Max), &(mPIDs[7]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_2Max), &(mPIDs[8]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_3Max), &(mPIDs[9]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, lam_4Max), &(mPIDs[10]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, mafMax), &(mPIDs[11]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, loadMax), &(mPIDs[12]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, tpsMax), &(mPIDs[13]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, ectMax), &(mPIDs[14]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, fpMax), &(mPIDs[15]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, mapMax), &(mPIDs[16]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, actMax), &(mPIDs[17]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, fuelMax), &(mPIDs[18]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, bapMax), &(mPIDs[19]->mMaxValue));
    eventPut(aExec, XINDEX(CApexDevice, vbatMax), &(mPIDs[20]->mMaxValue));

    setFlags(ExecuteOnEventUpdate);
}

// Make sure that aBuffer has APEX_MAX_LINE_LEN bytes.
int CApexDevice::readLine(char *aBuffer, int aTimeout)
{
    char *nl_pos = 0;
    int line_len;
    struct pollfd fd;
    
    fd.events = POLLIN;
    fd.revents = 0;
    fd.fd = mDescriptor;
    //    if (aTimeout > 0) {
	if (m1Poll(&fd, 1, aTimeout) <= 0) {
	    DBGFMT_CLS("CApexDevice::readLine(): Timeout when reading Apex data. Timeout [%d]", aTimeout);
	    return -1;
	}
	//	DBGFMT_CLS("readLine: poll returned data (tOut[%d])", aTimeout);
	//    }

    line_len = read(mDescriptor, aBuffer, APEX_MAX_LINE_LEN);
    if (line_len <= 0) {
	DBGFMT_CLS("CApexDevice::readLine(): Could not read data[%s]", strerror(errno));
	return -2;
    }
	
    // kill newline
    if (line_len > 0 && aBuffer[line_len - 1] == '\n')  {
	aBuffer[line_len-1] = 0;
	line_len--;
    }
    //    DBGFMT_CLS("CApexDevice::readLine(): Got [%s].", aBuffer);
    return line_len;
 }

CApexDevice::~CApexDevice(void)
{
    DBGFMT_CLS("CApexDevice::~CApexDevice(): Called");

    if (mDescriptor > -1) 
	close(mDescriptor);

    disconnect(XINDEX(CApexDevice,revents));
    m1Release(CFileSource, mSource);

    m1Release(CTimeout, mReadTimeoutTimer);
    m1Release(CTimeout, mPortReopenTimer);
}

bool CApexDevice::installPID(CExecutor* aExec, unsigned char aPID)
{
    int ind = 0;

    // We can either get 0x24 or 0x34 for lambda 1.
    // If we see 0x34, patch this PID into the mPIDs array.
    if (aPID == 0x34) 
	mPIDs[7]->mPID=0x34;

    if (aPID == 0x35)
	mPIDs[8]->mPID=0x35;

    if (aPID == 0x36) 
	mPIDs[7]->mPID=0x36;

    if (aPID == 0x37)
	mPIDs[8]->mPID=0x37;

    while(ind < APEX_PID_COUNT) {

	if (aPID == mPIDs[ind]->mPID) {
	    //
	    // Push back at correct round robin level.
	    //
	    mProperties[mPIDs[ind]->mLevel].push_back(mPIDs[ind]);
	    mPIDs[ind]->mSupported.putValue(aExec, 1);
	    return true;
	}
	++ind;
    }
    //    DBGFMT_CLS("CApexDevice::InstallPID(): PID[0x%.2X] has no match.", aPID);
    return false;
}

//
// Extract all supported PIDs
//
bool CApexDevice::getSupportedPIDs(CExecutor* aExec)
{
    char res[2048];
    int pid_offset = 0;
    int ind;
    DBGFMT_CLS("CApexDevice::GetSupportedPIDs(): Locating supported PIDs.");
    //
    // Clear the old properties tables.
    //
    mProperties[0].clear();
    mProperties[1].clear();
    mProperties[2].clear();
    mProperties[3].clear();
    
    while(1) {
	unsigned long bits;
	char buf[128];
	
	sprintf(buf, "01%.2X\r", pid_offset);
	write(mDescriptor, buf, 5);
	    
	if (readLine(res, 500) < 0)  {
	    DBGFMT_CLS("Could not read PID set.\n");
	    return false;
	}
	    

	strncpy(buf, res + 11, 8);
	buf[8] = 0;
	
	//	DBGFMT_CLS("CApexDevice::GetSupportedPIDs(): Got raw string[%s] PID field[%s]", res, buf);

	bits = strtoll(buf, 0, 16);
	//	DBGFMT_CLS("CApexDevice::GetSupportedPIDs(): Next PID long to process[0x%.8X]", bits);

	//
	// To retrieve multiple values over CAN.
	// C9 04 01 0c 0a 22
	//
	// Nissan 350Z 0100 resp:
	// V486B104100BF9FB991AC
	// Pontiac G5 G5 000 resp:
	// V486B104100BE3FB813AA
	//  
	// Nissan 350Z 010C resp:
	// ISO9141-2  DEST_ADDR  ECU_ADDR  RSP_CODE  RSP_BYTE_1(PID)  RSP_BYTE_2 (DATA1)  RSP_BYTE_2 (DATA2)  UNKNOWN
	// V48        6B         10        41        0C               16                  44                  6A
	// ------------------------------------------------------------------------------------------------------
	// 0                                         1
	// 012        34         56        78        90               12                  34                  56 
	// V486B10410C161238
	// V486B10410C157CA1
	//    DBGFMT("BITS: 0x%.4X", bits);

	ind = 32;
	while(ind--) {
	    if ((bits >> ind) & 1) {
		DBGFMT_CLS("PID 0x%.2X supported. Will try to install it", pid_offset + (32 - ind)); 
		installPID(aExec,  pid_offset + (32 - ind));
	    }
//   	    else
//   		DBGFMT_CLS("PID 0x%.2X not supported",  pid_offset + (32 - ind));
	}

	if (!(bits & 1)) {
	    //      puts("Bit 0 is not set. Done");
	    break;
	}
	pid_offset += 0x20;
    }

    DBGFMT_CLS("PID DUMP");
    for (ind = 0; ind < 4; ++ind) {
	DBGFMT_CLS("Level [%d] PIDs:", ind);
	for (CPIDListIterator iter = mProperties[ind].begin();
	     iter != mProperties[ind].end(); 
	     ++iter) {
	    DBGFMT_CLS("  PID[%.2X] Name[%s] Size[%d] PreAdd[%d] Multiplier[%f] PostAdd[%d]",
		       (*iter)->mPID,
		       (*iter)->mName,
		       (*iter)->mSize,
		       (*iter)->mPreAdd,
		       (*iter)->mMultiplier, 
		       (*iter)->mPostAdd);
	}
	DBGFMT_CLS("---");
    }

    //
    // Set mValue for all non supported PIDs to Minimum value - 1
    //
    ind = APEX_PID_COUNT;
    while(ind--) {
	if (!mPIDs[ind]->mSupported.value()) {
	    mPIDs[ind]->mSupported.putValue(aExec, 0); // Force scheduling.?
	}
    }

    //
    // Reset the iterators
    //
    mIterators[0] = mProperties[0].begin();
    mIterators[1] = mProperties[1].begin();
    mIterators[2] = mProperties[2].begin();
    mIterators[3] = mProperties[3].begin();
    mCurrent = mIterators[0];
    return true;
}
 
CApexDevice::CPID *CApexDevice::getNextPID(void)
{
    int ind = 0;
    CPID *res;

    //
    // Check that this PID is in use by the system.
    // If so, then break and request the PID.
    // We cannot use the subscriberCount() method here
    // since many inactive objects and instruments
    // may have a subscription to the PID while not
    // themselves being in use.
    //

    while(1) {
	while(mIterators[ind] == mProperties[ind].end()) {
	    mIterators[ind] = mProperties[ind].begin();
	    ++ind;

	    if (ind == 4) {
		ind = 0;
		break;
	    }
	}  

	if ((*mIterators[ind])->mUsage.value() > 0 || 
	    mCurrent == mIterators[ind])
	    break;

	mIterators[ind]++;
    }

    mCurrent = mIterators[ind];
    mIterators[ind]++;
//    printf("NextPID  [%s]\n", (*mCurrent)->mName);
    return *mCurrent;
}

bool CApexDevice::sendNextRequest(CExecutor *aExec, TimeStamp aTS)
{
     char req[128];

     // If we are in CAN mode, ask for the next six PIDs to use.
     // Batch mode does not really work yet...
//      if (mProtocol == CAN_11BIT_500K || mProtocol == CAN_11BIT_250K ||
// 	  mProtocol == CAN_29BIT_500K || mProtocol == CAN_29BIT_250K) {
     if (false) {
	  strcpy(req, "C90701");
	  for (int i = 0; i < 6; ++i) {
 	     CPID *pid = getNextPID();
	     char tmp[3];

	     sprintf(tmp, "%.2X", pid->mPID & 0xFF); // Bit ops some day..
	     strcat(req, tmp);
	  }
	  strcat(req, "\r");
	  DBGFMT_CLS("Sending CAN[%.18s]", req);
      } else {
	  CPID *pid = getNextPID();
	  sprintf(req, "01%.2X\r", pid->mPID & 0xFF);
	  //	  DBGFMT_CLS("apex: Sending Non-CAN[%.4s]", req);
      }
	 
      //
      // ind now points to the next mIterator to advance.
      //
      if (write(mDescriptor, req, strlen(req)) != strlen(req)) {
	  DBGFMT_CLS("CApexDevice::sendNextRequest(): Failed to write to Apex unit [%s]",  strerror(errno));
	  return false;
      }
      // Rst timeout timer.
      mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true));
      // tcflush(mDescriptor, TCIOFLUSH);
}

bool CApexDevice::processQueryProtocolResult(CExecutor* aExec, char *aResult) 
{
    DBGFMT_CLS("CApexDevice::queryProtocol(): Got init response[%s]", aResult);
    if (aResult[0] != 'P') {
	DBGFMT_CLS("CApexDevice::queryProtocol(): Unknown protocol query response [%s]", aResult);
	goto fail;
    }
    switch (aResult[1]) {
	case '0':
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex could not determine protocol");
	    goto fail;
		    
	case '1':
	    mProtocol.putValue(aExec, J1850_PWM);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl J1850 PWM");
	    break;

	case '2':
	    mProtocol.putValue(aExec, J1850_VPW);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl J1850 VPW");
	    break;

	case '3':
	    mProtocol.putValue(aExec, ISO_9141);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl ISO 9141");
	    break;

	case '4':
	    mProtocol.putValue(aExec, ISO_14230);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl ISO 14230");
	    break;

	case '5':
	    mProtocol.putValue(aExec, CAN_11BIT_500K);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl CAN 500Kb with 11 bit frames");
	    break;

	case '6':
	    mProtocol.putValue(aExec, CAN_29BIT_500K);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl CAN 500Kb with 29 bit frames");
	    break;

	case '7':
	    mProtocol.putValue(aExec, CAN_11BIT_250K);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl CAN 250Kb with 11 bit frames");
	    break;

	case '8':
	    mProtocol.putValue(aExec, CAN_29BIT_250K);
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device reported protocl CAN 250Kb with 29 bit frames");
	    break;
		    
	default:
	    DBGFMT_CLS("CApexDevice::queryProtocol(): Apex device replied with unknown protocol [%c]", aResult[1]);
	    goto fail;
    }

    mState.putValue(aExec, APEX_RUNNING);
    return true;

 fail:
    mProtocol.putValue(aExec, UNKNOWN);
    return false;
}

CApexDevice::CPID *CApexDevice::findPID(char *aBuffer)
{
    char tmp[3];
    int pid;
    tmp[0] = *aBuffer++;
    tmp[1] = *aBuffer++;
    tmp[2] = 0;

    pid = strtol(tmp, 0, 16);

    for (int ind = 0; ind < APEX_PID_COUNT; ++ind) 
	if (pid == mPIDs[ind]->mPID)
	    return mPIDs[ind];

    return 0;
}

int CApexDevice::setPID(CExecutor* aExec, char *aBuffer) 
{
    char tmp[10];

    CPID *pid = findPID(aBuffer);
    if (!pid) {
// 	DBGFMT_WARN("Could not locate PID[0x%c%c]/[%s]",
// 		    aBuffer[0], aBuffer[1], aBuffer);
	return 0;
    }

//     printf("Found PID [0x%2X] [%s]\n",
// 	   pid->mPID, aBuffer);

    aBuffer += 2; // Move past PID.
    switch(pid->mSize) {
    case 1:
	tmp[0] = *aBuffer++;
	tmp[1] = *aBuffer++;
	tmp[2] = 0;
	break;

    case 2:
	tmp[0] = *aBuffer++;
	tmp[1] = *aBuffer++;
	tmp[2] = *aBuffer++;
	tmp[3] = *aBuffer++;
	tmp[4] = 0;
	break;

    case 4:
	tmp[0] = *aBuffer++;
	tmp[1] = *aBuffer++;
	tmp[2] = *aBuffer++;
	tmp[3] = *aBuffer++;
	tmp[4] = *aBuffer++;
	tmp[5] = *aBuffer++;
	tmp[6] = *aBuffer++;
	tmp[7] = *aBuffer++;
	tmp[8] = 0;
	break;
    deafault:
	tmp[0] = 0;
    }	

    //
    // Special case.
    // The lambda pid contains voltage in bytes 0..1 and 
    // percent in bytes 2..3.
    // For now, we will only use percent.
    //
    if (pid->mPID == 0x24 || pid->mPID == 0x25 ||  // Lam1 or lam2, lam3, lam4
	pid->mPID == 0x26 || pid->mPID == 0x27 ||
	pid->mPID == 0x34 || pid->mPID == 0x35 ||
	pid->mPID == 0x36 || pid->mPID == 0x37) {
    /// mp[0..1] contains the equivalence ratio (0-2)
	// tmp[2..3] contains the voltage (not use)
	tmp[4] = 0;
    }
	
    pid->mValue.putValue(aExec,
			 float(pid->mPreAdd + strtol(tmp, 0, 16)) * 
			 pid->mMultiplier + float(pid->mPostAdd));

//          DBGFMT_CLS("CApexDevice::setPID(): [%s] PreAdd[%f] + (Val[%f] * Mul[%f]) + PostAdd[%d] = %f Size:[%d]", 
//     	    pid->mName,
//   	    float(pid->mPreAdd),
//     	    float(strtol(tmp, 0, 16)),
//     	    float(pid->mMultiplier),
//     	    pid->mPostAdd,
//     	    pid->mValue.self(),
//     	    pid->mSize);
    
    return pid->mSize * 2 + 2;
}

void CApexDevice::openPort(CExecutor* aExec, TimeStamp aTimeStamp)
{
    char res[100];
    struct termios t;

    // Rst timer.
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));

    if (mDescriptor != -1)
	close(mDescriptor);

    DBGFMT_CLS("CApexDevice::openPort(): (Re)opening device [%s] for Apex communication.", mPort.value().c_str());
    mDescriptor = open(mPort.value().c_str(), O_RDWR);

    if (mDescriptor == -1) {
	DBGFMT_CLS("CApexDevice:openPort(): Could not open [%s]: %s", mPort.value().c_str(), strerror(errno));
	if (mPort.value() == "sova")
	    usleep(800000);
	goto fail;
    }


    //
    // Setup device
    //
    tcgetattr(mDescriptor, &t);
    t.c_cflag = CS8 | CLOCAL | CREAD;
    t.c_iflag = IGNBRK | IGNPAR | IGNCR; // Ignore break, parity and map cr->nl
    t.c_oflag = ONLCR ;  // nl->cr
    t.c_lflag = ICANON; // Canonical mode. Raw mode eats more cpu.
    t.c_cc[VINTR] = 0;
    t.c_cc[VQUIT] = 0;
    t.c_cc[VERASE] = 0;
    t.c_cc[VEOF] = 0;
    t.c_cc[VEOL] = '\r';
    t.c_cc[VEOL2] = 0;
    t.c_cc[VSWTC] = 0;
    t.c_cc[VSTOP] = 0;
    t.c_cc[VSUSP] = 0;
    t.c_cc[VLNEXT] = 0;
    t.c_cc[VWERASE] = 0;
    t.c_cc[VREPRINT] = 0;
    t.c_cc[VDISCARD] = 0;

    cfsetospeed(&t, B115200);
//     t.c_iflag = IGNBRK | IGNPAR | IGNCR;
//     t.c_oflag = ONLCR;

//    t.c_cflag = CS8 | CLOCAL | CREAD;
    //    t.c_lflag = ICANON;
 
    if (tcsetattr(mDescriptor, TCSANOW, &t) == -1) {
	DBGFMT_CLS("CApexDevice:openPort(): Could not set termio on [%s]: %s",  mPort.value().c_str(), strerror(errno));
	goto fail;
    }

    mSource->setDescriptor(aExec, mDescriptor, POLLIN);

    //
    // Setup a request to initiate OBD-II communication
    // 

    //
    // Flush the buffer
    //
    tcflush(mDescriptor, TCIOFLUSH);
    write(mDescriptor, "\r",1); //Rst
    usleep(100000);
    write(mDescriptor, "\r",1); //Rst
    usleep(100000);
    write(mDescriptor, "D0\r", 3); //Rst
    usleep(500000);
    write(mDescriptor, "\r",1); //Rst
    usleep(100000); 
    write(mDescriptor, "\r",1); //Rst
	
    while (readLine(res, 300) != -1);
	

    DBGFMT_CLS("CApexDevice::openPort(): Got <CR> response[%s]", res);
    if (strncmp(res, "OBDScan", 7)) {
	DBGFMT_CLS("CApexDevice::openPort(): This is not an Apex unit. Got response[%s]");
	goto fail;
    }

    //
    // If mPlotType is unknown. Query the Apex adapter.
    //
    if (mProtocol.value() == UNKNOWN) {
	DBGFMT_CLS("CApexDevice::openPort(): protocol is unknown. Querying apex about it");
	if (write(mDescriptor, "93\r", 3) != 3) {
	    DBGFMT_CLS("CApexDevice::openPort(): Could not query protocol.");
	    goto fail;
	}
	mState.putValue(aExec, APEX_QUERYING_PROTOCOL);
    } else {
	DBGFMT_CLS("CApexDevice::openPort(): protocol is preset to [%.2d]", mProtocol.value());
	sprintf(res, "9200%.2d\r", mProtocol.value());
	DBGFMT_CLS("CApexDevice::openPort(): Sending %s", res);
	write(mDescriptor, res , 7);
	res[0] = 0;
	readLine(res, 200);
	if (res[0] != 'V' || res[1] != 'P') {
	    DBGFMT_CLS("CApexDevice::openPort(): Apex device did not accept preset protocol[%.6s]. Responded with[%s]", res);
	    goto fail;
	}		

	//
	// Retrieve PIDs
	//
	if (!getSupportedPIDs(aExec)) {
	    DBGFMT_CLS("CApexDevice::openPort(): Could not retrieve PIDs");
	    goto fail;
	}
	mState.putValue(aExec, APEX_RUNNING);

	// Enable read timeout
	mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
	sendNextRequest(aExec, aTimeStamp);

    }
    DBGFMT_CLS("CApexDevice::openPort(): Done.");
    return;

 fail:
    DBGFMT_CLS("CApexDevice::openPort(): Failed. Will try to reopen later.");
    mState.putValue(aExec, APEX_NO_CONNECTION);
    //    mState.tracePropagationOn();
    mProtocol.putValue(aExec, UNKNOWN);
    if (mDescriptor != -1) {
	close(mDescriptor);
	mDescriptor = -1;
	mSource->setDescriptor(aExec, -1);
    }

    // Setup a reopen time
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true));
}

void CApexDevice::execute(CExecutor* aExec)
{
    TimeStamp tTimeStamp = aExec->cycleTime();
    char req[15];
    char res[APEX_MAX_LINE_LEN];
    char can_res[512];
    int tmp_hex;
    char tmp[APEX_MAX_LINE_LEN];
    int val, current_val;
    bool changed = false;
    int read_len;
    int frame_count = 0;
    int ind;


    if (mPortReopenTimeout.updated() && mPortReopenTimeout.value() == true)  {
	DBGFMT_CLS("Port reopen timeout. Trying to reopen port.");
	openPort(aExec, tTimeStamp);
    }
    
	
    //
    // Port is updated. Close old descriptor and open new one.
    //
    if (mPort.updated()) {
	DBGFMT_CLS("port field updated to [%s]. Will open.", mPort.value().c_str());
	openPort(aExec, tTimeStamp);
	if (mState.value() == APEX_NO_CONNECTION)
	    return;
    }

    //
    // Protocol is updated and we have an active port.
    //
    if (mProtocol.updated() && mDescriptor != -1 && 
	mState.value() != APEX_QUERYING_PROTOCOL)  {
	if (mProtocol.value() == UNKNOWN) {
	    DBGFMT_CLS("CApexDevice::execute(): protocol is set to unknown and device is open. Will query protocol.", mProtocol.value());
	    mState.putValue(aExec, APEX_QUERYING_PROTOCOL);
	    if (write(mDescriptor, "93\r", 3) != 3) {
		DBGFMT_CLS("CApexDevice::open_port(): Could not query protocol.");
		goto fail;
	    }

	    return;
	}
	DBGFMT_CLS("CApexDevice::execute(): protocol is updated to [%d] and device is open. Will set new protocol.", mProtocol.value());
	sprintf(req, "9200%.2d\r", mProtocol.value());
	write(mDescriptor, req , 7);
	res[0] = 0;
	readLine(res, 200);
	if (res[0] != 'V' || res[1] != 'P') {
	    DBGFMT_CLS("CApexDevice::execute(): Apex device did not accept preset protocol[%.6s]. Responded with[%s]", res);
	    goto fail;
	}		
	write(mDescriptor, "\r", 1);
	readLine(res, 200);
	write(mDescriptor, "\r", 1);
	readLine(res, 200);
	write(mDescriptor, "\r", 1);
	readLine(res, 200);
	write(mDescriptor, "\r", 1);
	readLine(res, 200);
	mState.putValue(aExec, APEX_RUNNING);

	// Set iso gap timing if applicable.
	if ((mProtocol.value() == ISO_14230 || 
	     mProtocol.value() == ISO_9141) && 
	    mIsoGapTiming.value() != 0)
	    mIsoGapTimingPending = true;
    }

    //
    // Check if we need to schedule an iso gap timing update.
    //
    if (mIsoGapTiming.updated() && mIsoGapTiming.value() != 0) {
	DBGFMT_CLS("CApexDevice::execute(): Setting ISO gap timing[%u] as pending", mIsoGapTiming.value());
	mIsoGapTimingPending = true;
    }
	
    if (mDescriptor == -1)
	return;

    
    if (mReadTimeout.updated() && mReadTimeout.value() == true) {
	DBGFMT_CLS("WTF!");

	// Allow for three read failures before giving up.
	mReadFailCount++;
	if (mReadFailCount == 5)
	    goto fail;

	mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
	sendNextRequest(aExec, tTimeStamp);
	return;
    }	
    
    if (!mRevents.updated()) {
	DBGFMT_CLS("No input.");
	return;
    }	

    //  dumptime("SENDR");
    //
    // Check if the port is not open.
    // If this is the case, attempt to reopen the port
    // and init the device and return.
    // The last thing that SetMember will do is to call
    // SendNextRequest to initiate a new request.
    //
  
    //
    // Check that we were able to read a reply.
    // If not move into disconnected mode.
    //[V416B1041058C

    res[0] = 0;
    read_len = readLine(res, 0);
    if (read_len == -2) {
	DBGFMT_CLS("CApexDevice::execute(): Failed to read from device.");
	goto fail;
    }

    if (read_len == -1) {
	DBGFMT_CLS("CApexDevice::execute(): Null read. Retry.");

	// Allow for three read failures before giving up.
	mReadFailCount++;
	if (mReadFailCount == 5)
	    goto fail;

	goto done;
    }
	
    mReadFailCount = 0;
    //
    // Check if we are querying protocols.
    //
    if (mState.value() == APEX_QUERYING_PROTOCOL) {
	DBGFMT_CLS("Processing protocol query result.");
	if (!processQueryProtocolResult(aExec, res)) {
	    DBGFMT_CLS("processQueryProtocolResult(res) fail");
	    goto fail;
	}

	DBGFMT_CLS("Getting PIDs");
	getSupportedPIDs(aExec);
	DBGFMT_CLS("CApexDevice::execute(): Protocol obtained. Starting request cycle.");
	mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
	sendNextRequest(aExec, tTimeStamp);
	return;
    }
    
    //    DBGFMT_CLS("apex: Got[%s]", res);

    if (strlen(res) < 13) {
	DBGFMT_CLS("CApexDevice::execute(): Illegal response[%s]", res);
	goto done;
    }

    //
    // update latency. FIXME - Configurable timeout, not static 0.5 sec.
    //
    mLatency.putValue(aExec, 0.5 - mReadTimeoutTimer->remain());
    
    
    // If this is not a CAN protocol, set the single PID and return.
    // Batch read does not really work yet, so we always jump here.
//     if (mProtocol != CAN_11BIT_500K && mProtocol != CAN_11BIT_250K &&
// 	mProtocol != CAN_29BIT_500K && mProtocol != CAN_29BIT_250K) {
    if (true) {
//	printf("Setting non CAN PID\n");
	setPID(aExec, res + 9);
	goto done;
    }

    // 
    // This is CAN protocol response, broken down into frames.
    // Each frame is 24 bytes.
    //
    frame_count = strlen(res) / 24;
//     printf("Response length[%d]. That is [%d] frames\n",  strlen(res), frame_count);

    //
    // Stitch together a response sequence PPVVPPVVVVPPVVPPVVVV
    // where we have dropped the frame headers and only have PP=PID
    // VV=2 byte value and VVVV 4 byte PID value.
    //
    ind = 0;
    for(int frame_ind = 0; frame_ind < frame_count; ++frame_ind) {
	// ptr will point to start of frame. Skip the frame header of 10 bytes.
	char *ptr = res + frame_ind * 24 + 10;
	for (int i = 0; i < 14; ++i)
	    can_res[ind++] = *ptr++;
    }
    can_res[ind] = 0;
    ind = 0;
//    printf("CAN res[%s]\n", can_res);
    while(can_res[ind]) {
	CPID *pid = findPID(&can_res[ind]);
	int advance;

	//
	// Weird bug workaround. On ford, at least,
	// the first pid always has the value 41. Skip this
	// I have no idea why...
	//
	if (!ind) {
	    ind += 4;
	    continue;
	}
	advance = setPID(aExec, &can_res[ind]);
	if (!advance) 
	    goto done;
		
	ind += advance;
    }

 done:
    // Check if we should set a new Iso Gap timing value
    if (mIsoGapTimingPending) {
	if  (mProtocol.value() == ISO_9141 || mProtocol.value() == ISO_14230) {
	    char buf[128];
	    sprintf(buf, "98%.2u\r", mIsoGapTiming.value());
	    write(mDescriptor, buf, 5);
	    buf[strlen(buf) - 1 ] = 0;
	    DBGFMT_CLS("Set ISO gap timing to [%u] (Command: [%s]", mIsoGapTiming.value(), buf);
	    readLine(buf, 400);
	    DBGFMT_CLS("ISO gap timing response [%s]\n", buf);
	}
	mIsoGapTimingPending = false;
    }
    sendNextRequest(aExec, tTimeStamp);
    
    return;

 fail:
    DBGFMT_CLS("FAIL\n");
    mState.putValue(aExec, APEX_NO_CONNECTION);
    if (mDescriptor != -1) {
	close(mDescriptor);
	mDescriptor = -1;
	mSource->setDescriptor(aExec, -1);
    }

    mProtocol.putValue(aExec, UNKNOWN);
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));

    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true));
    return;
}


