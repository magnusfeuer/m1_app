//
// All rights reserved. Reproduction, modification, use or disclosure
//n to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "zt2_device.hh"
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

XOBJECT_TYPE_BOOTSTRAP(CZT2Device);

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


CZT2Device::CZT2Device(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDescriptor(-2),
    mPort(this),
    mState(this),
    mCylinderCount(this),
    mRPMMultiplier(this),
    mRevents(this),
    mSource(NULL),
    mInputBufferLen(0),
    mFirstRead(false),
    mLam(this),
    mEgt(this),
    mRpm(this),
    mMap(this),
    mTps(this),
    mUser1(this)
{ 
    eventPut(aExec, XINDEX(CZT2Device, revents), &mRevents);  // File revents trigger event.
    eventPut(aExec, XINDEX(CZT2Device, port), &mPort);
    mState.putValue(aExec, NO_CONNECTION);
    eventPut(aExec, XINDEX(CZT2Device, state), &mState);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);
    connect(XINDEX(CZT2Device,revents), mSource, XINDEX(CFileSource,revents));



    mCylinderCount.putValue(aExec, 1);
    eventPut(aExec, XINDEX(CZT2Device, cylinder_count), &mCylinderCount);
    mRPMMultiplier.putValue(aExec, 1.0);
    eventPut(aExec, XINDEX(CZT2Device, rpm_multiplier), &mRPMMultiplier);

    //
    // Setup channels.
    //
    mLam.putValue(aExec, 1.0);
    mEgt.putValue(aExec, 0.0);
    mRpm.putValue(aExec, 0.0);
    mMap.putValue(aExec, 0.0);
    mTps.putValue(aExec, 0.0);
    mUser1.putValue(aExec, 0.0);
    eventPut(aExec, XINDEX(CZT2Device, lam), &mLam);
    eventPut(aExec, XINDEX(CZT2Device, egt), &mEgt);
    eventPut(aExec, XINDEX(CZT2Device, rpm), &mRpm);
    eventPut(aExec, XINDEX(CZT2Device, map), &mMap);
    eventPut(aExec, XINDEX(CZT2Device, tps), &mTps);
    eventPut(aExec, XINDEX(CZT2Device, user1), &mUser1);
    setFlags(ExecuteOnEventUpdate);
}

bool CZT2Device::getData(void)
{
    int res;
    struct pollfd fd;
    
    fd.events = POLLIN;
    fd.revents = 0;
    fd.fd = mDescriptor;
    if (m1Poll(&fd, 1, 0) <= 0) {
	DBGFMT_CLS("CZT2Device.::getData() False positive.");
	return false;
    }


    //
    // Check for buffer overflow.
    //
    if (mInputBufferLen >= ZT2_INPUT_BUFFER_SIZE) {
	DBGFMT_WARN("CZT2Device::getData(): Input buffer overlow!");
	return true;
    }

    if ((res = read(mDescriptor, mInputBuffer + mInputBufferLen, ZT2_INPUT_BUFFER_SIZE - mInputBufferLen)) <= 0) {
	DBGFMT_WARN("Failed to read from device: %s", strerror(errno));
	return false;
    }
    mInputBufferLen += res;

    //    DBGFMT_CLS("CZT2Device::getData(): Got %d bytes. Total len %d bytes", res, mInputBufferLen);

    return true;
}


//
// Dump all bytes up to the next block starter.
// Extract 14 bytes and put in aBuffer.
// Return true if successful, 0 if not enough data is
// available.
//
bool CZT2Device::extractData(uchar *aBuffer, int aLength)
{
    int ind = 0;

    if (aLength < 3)
	return false;

    //
    // Skip until we have sync
    //
    while(mInputBufferLen - ind >= aLength &&
	  (mInputBuffer[ind]  != uchar(0x00) || 
	   mInputBuffer[ind + 1] != uchar(0x01) || 
	   mInputBuffer[ind + 2] != uchar(0x02))) {
	DBGFMT_CLS("buf[%d]=0x%.2X  buf[%d]=0x%.2X  buf[%d]=0x%.2X", 
		   ind, mInputBuffer[ind] & 0xFF,
		   ind + 1, mInputBuffer[ind + 1] & 0xFF,
		   ind + 2, mInputBuffer[ind + 2] & 0xFF);
	++ind;
    }

	
    // Not enough
    if (mInputBufferLen - ind < aLength) {
	//	DBGFMT_CLS("CZT2Device::extractData(): Wanted[%d] bytes. Have[%d] ind[%d]", aLength, mInputBufferLen, ind);
	return false;
    }
    if (ind > 0) 
	DBGFMT_CLS("CZT2Device::extractData(): Skipped [%d] bytes to sync.", ind);


    memcpy(aBuffer, mInputBuffer + ind, aLength);
    mInputBufferLen -= (aLength + ind);
    if (mInputBufferLen > 0) 
	memmove(mInputBuffer, mInputBuffer + aLength + ind, mInputBufferLen);

    return true;
}
    
CZT2Device::~CZT2Device(void)
{
    DBGFMT_CLS("CZT2Device::~CZT2Device(): Called");

    if (mDescriptor >= 0) 
	close(mDescriptor);

    disconnect(XINDEX(CZT2Device,revents));
    m1Release(CFileSource, mSource);
}

 


bool CZT2Device::processStream(CExecutor* aExec) 
{
    uchar buf[ZT2_INPUT_BUFFER_SIZE];
    int res = 0;
    float raw_map;


    //
    // There is usually 9Kbytes of data that has been received
    // while system is initializing.
    // Dump that.
    //
    if (mFirstRead) {
	tcflush(mDescriptor, TCIOFLUSH);
	mFirstRead = false;
	return true;
    }
	
    //
    // Data should be readily available since we triggered on poll.
    // 
    if (!getData())
	return true;

    //
    // Rst timeout.
    //
    while (extractData(buf, 14)) {
	++res;
	// buf[0-2] are sync bytes. Skip.

	if (buf[0] != uchar(0x00) || buf[1] != uchar(0x01) || buf[2] != uchar(0x02)) {
	    DBGFMT_CLS("CZT2Device::processStream(): SYNC FAIL!\n");
	    return false;
	}
	    
	mLam.putValue(aExec, (float) buf[3] / 147.0) ; // Convert to lambda.
	mEgt.putValue(aExec, (float) (buf[4] | (buf[5] << 8)));// EGT
	//	printf("CZT2Device:: mEGT[%f] 0x%.2X%.2X\n", mEgt.value(), buf[5] & 0xFF, buf[4] & 0xFF);


	// Check if engine is running at all.
	if ((buf[6] | (buf[7] << 8)) <= 0x2000)
	    mRpm.putValue(aExec, (roundf((1000000.0 / (float) (buf[6] | (buf[7] << 8)))) * 4.59 )/ 
			  (float) mCylinderCount.value() * mRPMMultiplier.value());
	else
	    mRpm.putValue(aExec,0.0);
	
	// Check if MAP is reporting a vacuum (bit 8 set), or boost.
	raw_map = float(buf[8] | ((buf[9] & 0x7F) << 8));	    
	
	///	printf("0x%.4X\n", buf[8] | (buf[9] << 8) & 0xFFFF);
  	if ((buf[9] & 0x80) == 0x80) {
	    mMap.putValue(aExec, 0.0 - float(raw_map)); 
	    //	    printf("CZ2Device::vacuum: raw[%d] inHgi[%f] = mbar[%f]\n", (buf[8] | (buf[9] << 8)) & 0xFFFF, raw_map, mMap.value());
	}
	else {
	    mMap.putValue(aExec, float(raw_map));
	    //	    printf("CZ2Device::boost: raw[%d] psi[%f] = mbar[%f]\n",  (buf[8] | (buf[9] << 8)) & 0xFFFF, raw_map, mMap.value());
	}
	//	printf("CZ2Device::vacuum: raw[%f]\n", mMap.value());/

	mTps.putValue(aExec, float(buf[10]) * 2.5);
	//	printf("CZ2Device:: tps[%f]\n", mTps.value());
	
	//	DBGFMT_CLS("CZ2Device::boost: tps[%d] ",  buf[10]& 0xFF);
	mUser1.putValue(aExec, float (buf[11] & 0xFF));
	//	DBGFMT_CLS("CZ2Device::processStream(): mUser1[%f]", mUser1.value());
// 	DBGFMT_CLS("CZT2Device::processStream(): cylCount[%d] raw rpm[0x%.4X] rpm[%f] egt[%d]!",
// 		   mCylinderCount.value(), 
// 		   (int) ((buf[6] | (buf[7] << 8)) & 0xFFFF), 
// 		   mRpm.value(),
// 		   (int)  (buf[4] | (buf[5] << 8)) & 0xFFFF);

    }
    return true;
}

void CZT2Device::openPort(CExecutor* aExec)
{
    uchar res[100];
    struct termios t;


    if (mDescriptor >= 0)
	close(mDescriptor);

    if ((mDescriptor = open(mPort.value().c_str(), O_RDWR|O_NOCTTY)) < 0) {
	DBGFMT_CLS("CZT2Device:openPort(): Could not open [%s]: %s", 
		   mPort.value().c_str(), strerror(errno));
	goto fail;	
    }
    DBGFMT_CLS("CZT2Device:openPort(): mDescriptor=%d", mDescriptor);

    //
    // Setup device
    //
    tcgetattr(mDescriptor, &t);
    t.c_iflag = IGNBRK | IGNPAR;
    t.c_oflag = 0;

    t.c_cflag = CS8 | CLOCAL | CREAD;
    cfsetospeed(&t, B9600);
    t.c_lflag = 0;
    t.c_cc[VTIME] = 0;
    t.c_cc[VMIN] = 0;
    cfmakeraw(&t);
 
    if (tcsetattr(mDescriptor, TCSANOW, &t) == -1) {
	DBGFMT_WARN("CZT2Device:openPort(): Could not set termio on [%s]: %s",  mPort.value().c_str(), strerror(errno));
	goto fail;
    }

    mSource->setDescriptor(aExec, mDescriptor, POLLIN);
    DBGFMT_CLS("CZT2Device::openPort(): Done.");
    mState.putValue(aExec, RUNNING);
    mFirstRead = true;
    return;

 fail:
    DBGFMT_CLS("CZT2Device::openPort(): Failed. Will try to reopen later.");
    mState.putValue(aExec, NO_CONNECTION);
    if (mDescriptor < 0) {
	close(mDescriptor);
	mDescriptor = -1;
	mSource->setDescriptor(aExec, -1);
    }

    //
    // Setup a reopen time
    //
    mFirstRead = false;
    return;
}

void CZT2Device::start(CExecutor* aExec) 
{
}

void CZT2Device::execute(CExecutor* aExec)
{
    //
    // Port is updated. 
    // Close old descriptor and open new one.
    //
    if (mDescriptor == -2 && mPort.value() != "") {
	DBGFMT_CLS("port field updated to [%s]. Will open.", mPort.value().c_str());
	openPort(aExec);
    }

     if (mDescriptor == -1)
	return;
    
    if (!mRevents.updated()) {
	DBGFMT_CLS("No input.");
	return;
    }	

    if (!processStream(aExec))
	goto fail;


    return;

 fail:
    DBGFMT_CLS("FAIL\n");
    mState.putValue(aExec, NO_CONNECTION);
    if (mDescriptor != -1) {
	close(mDescriptor);
	mDescriptor = -1;
	mSource->setDescriptor(aExec, -1);
    }


    mFirstRead = false;
    return;
}


