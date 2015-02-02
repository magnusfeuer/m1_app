//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "carpal_device.hh"
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

XOBJECT_TYPE_BOOTSTRAP(CCarPalDevice);


CCarPalDevice::ProtocolDescriptor CCarPalDevice::mProtocols[] = {
    { ISO_9141_0808,   "ISO_9141_0808",    11 },
    { ISO_9141_9494,   "ISO_9141_9494",    11 },
    { ISO_14230_SLOW,  "ISO_14230_SLOW",   11 },
    { ISO_14230_FAST,  "ISO_14230_FAST",   11 },
    { J1850_PWM,       "J1850_PWM",        11 },
    { J1850_VPW,       "J1850_VPW",        11 },
    { CAN_11BIT_250K,  "CAN_11BIT_250K",   11 },
    { CAN_11BIT_500K,  "CAN_11BIT_500K",   11 },
    { CAN_29BIT_500K,  "CAN_29BIT_500K",   15 },
    { CAN_29BIT_250K,  "CAN_29BIT_250K",   15 },
    { SAE_J1939,       "SAE_J1939",        0  },
    { KW_1281,         "KW_1281",          0  },
    { KW_82,           "KW_82",            0  },
    { UNKNOWN,         "UNKNOWN",          0  }
};


CCarPalDevice::CCarPalDevice(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mCurrentPID(0),
    mDescriptor(-1),
    mPort(this),
    mProtocol(this),
    mState(this),
    mRevents(this),
    mSource(NULL),
    mPortReopenTimer(NULL),
    mPortReopenTimeout(this),
    mReadTimeoutTimer(NULL),
    mReadTimeout(this),
    mInputBufferLen(0),
    mLatency(this),
    mReadFailCount(0),
    mPidOffset(0)
{ 
    int i;
    
    mProtocol.putValue(aExec, UNKNOWN);

    mPIDs[0] =  new CPID(aExec, 2, "s_trim_1", this, (unsigned char) 0x06, 1, -128, 0.7812, 0, -100, 100);
    mPIDs[1] =  new CPID(aExec, 3, "l_trim_1", this , (unsigned char) 0x07, 1, -128, 0.7812, 0, -100, 100 );
    mPIDs[2] =  new CPID(aExec, 2, "s_trim_2", this , (unsigned char) 0x08, 1, -128, 0.7812, 0, -100, 100 );
    mPIDs[3] =  new CPID(aExec, 3, "l_trim_2", this , (unsigned char) 0x09, 1, -128, 0.7812, 0, -100, 100 );
    mPIDs[4] =  new CPID(aExec, 0, "timing", this , (unsigned char) 0x0E, 1, 0, 0.5, -65, -64, 64 );
    mPIDs[5] =  new CPID(aExec, 0, "rpm", this , (unsigned char) 0x0C, 2, 0, 0.25, 0, 0, 16383 );
    mPIDs[6] =  new CPID(aExec, 1, "spd", this , (unsigned char) 0x0D, 1, 0, 1.0,  0, 0, 255 );
    mPIDs[7] =  new CPID(aExec, 0, "lam_1", this , (unsigned char) 0x24, 2, 0, 0.005, 0, 0, 99.2 );
    mPIDs[8] =  new CPID(aExec, 0, "lam_2", this , (unsigned char) 0x25, 2, 0, 0.005, 0, 0, 99.2 );
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

    eventPut(aExec, XINDEX(CCarPalDevice, protocol), &mProtocol);  // File revents trigger event.
    eventPut(aExec, XINDEX(CCarPalDevice, revents), &mRevents);  // File revents trigger event.

    mLatency.putValue(aExec, 0.0);
    eventPut(aExec, XINDEX(CCarPalDevice, latency), &mLatency);  // ECU response time.

    eventPut(aExec, XINDEX(CCarPalDevice, port), &mPort);
    mState.putValue(aExec, UNDEFINED);
    eventPut(aExec, XINDEX(CCarPalDevice, state), &mState);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);
    connect(XINDEX(CCarPalDevice,revents), mSource, XINDEX(CFileSource,revents));

    //
    // Create port reopen timer.
    //
    mPortReopenTimer = m1New(CTimeout, aExec);
    m1Retain(CTimer, mPortReopenTimer);
    aExec->addComponent(mPortReopenTimer);
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(1.0));

    eventPut(aExec, XINDEX(CCarPalDevice, port_reopen_timeout), &mPortReopenTimeout);
    connect(XINDEX(CCarPalDevice, port_reopen_timeout), mPortReopenTimer, XINDEX(CTimeout, timeout));

    //
    // Create read timeout timer.
    //
    mReadTimeoutTimer = m1New(CTimeout, aExec);
    m1Retain(CTimeout, mReadTimeoutTimer);
    aExec->addComponent(mReadTimeoutTimer);

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(0.5));

    eventPut(aExec, XINDEX(CCarPalDevice, read_timeout), &mReadTimeout);
    connect(XINDEX(CCarPalDevice, read_timeout), mReadTimeoutTimer, XINDEX(CTimeout, timeout));

    for (i = 0; i < CARPAL_PID_COUNT; i++) {
	mPIDs[i]->mValue.putValue(aExec, mPIDs[i]->mMinValue.value());
	mPIDs[i]->mUsage.putValue(aExec, 0);
    }

    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_1), &(mPIDs[0]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_1), &(mPIDs[1]->mValue)); 
    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_2), &(mPIDs[2]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_2), &(mPIDs[3]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, timing), &(mPIDs[4]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, rpm), &(mPIDs[5]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, spd), &(mPIDs[6]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_1), &(mPIDs[7]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_2), &(mPIDs[8]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_3), &(mPIDs[9]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_4), &(mPIDs[10]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, maf), &(mPIDs[11]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, load), &(mPIDs[12]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, tps), &(mPIDs[13]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, ect), &(mPIDs[14]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, fp), &(mPIDs[15]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, map), &(mPIDs[16]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, act), &(mPIDs[17]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, fuel), &(mPIDs[18]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, bap), &(mPIDs[19]->mValue));
    eventPut(aExec, XINDEX(CCarPalDevice, vbat), &(mPIDs[20]->mValue));


    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_1Supported), &(mPIDs[0]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_1Supported), &(mPIDs[1]->mSupported)); 
    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_2Supported), &(mPIDs[2]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_2Supported), &(mPIDs[3]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, timingSupported), &(mPIDs[4]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, rpmSupported), &(mPIDs[5]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, spdSupported), &(mPIDs[6]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_1Supported), &(mPIDs[7]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_2Supported), &(mPIDs[8]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_3Supported), &(mPIDs[9]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_4Supported), &(mPIDs[10]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, mafSupported), &(mPIDs[11]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, loadSupported), &(mPIDs[12]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, tpsSupported), &(mPIDs[13]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, ectSupported), &(mPIDs[14]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, fpSupported), &(mPIDs[15]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, mapSupported), &(mPIDs[16]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, actSupported), &(mPIDs[17]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, fuelSupported), &(mPIDs[18]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, bapSupported), &(mPIDs[19]->mSupported));
    eventPut(aExec, XINDEX(CCarPalDevice, vbatSupported), &(mPIDs[20]->mSupported));


    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_1Usage), &(mPIDs[0]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_1Usage), &(mPIDs[1]->mUsage)); 
    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_2Usage), &(mPIDs[2]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_2Usage), &(mPIDs[3]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, timingUsage), &(mPIDs[4]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, rpmUsage), &(mPIDs[5]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, spdUsage), &(mPIDs[6]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_1Usage), &(mPIDs[7]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_2Usage), &(mPIDs[8]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_3Usage), &(mPIDs[9]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_4Usage), &(mPIDs[10]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, mafUsage), &(mPIDs[11]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, loadUsage), &(mPIDs[12]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, tpsUsage), &(mPIDs[13]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, ectUsage), &(mPIDs[14]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, fpUsage), &(mPIDs[15]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, mapUsage), &(mPIDs[16]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, actUsage), &(mPIDs[17]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, fuelUsage), &(mPIDs[18]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, bapUsage), &(mPIDs[19]->mUsage));
    eventPut(aExec, XINDEX(CCarPalDevice, vbatUsage), &(mPIDs[20]->mUsage));

    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_1Min), &(mPIDs[0]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_1Min), &(mPIDs[1]->mMinValue)); 
    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_2Min), &(mPIDs[2]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_2Min), &(mPIDs[3]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, timingMin), &(mPIDs[4]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, rpmMin), &(mPIDs[5]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, spdMin), &(mPIDs[6]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_1Min), &(mPIDs[7]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_2Min), &(mPIDs[8]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_3Min), &(mPIDs[9]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_4Min), &(mPIDs[10]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, mafMin), &(mPIDs[11]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, loadMin), &(mPIDs[12]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, tpsMin), &(mPIDs[13]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, ectMin), &(mPIDs[14]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, fpMin), &(mPIDs[15]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, mapMin), &(mPIDs[16]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, actMin), &(mPIDs[17]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, fuelMin), &(mPIDs[18]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, bapMin), &(mPIDs[19]->mMinValue));
    eventPut(aExec, XINDEX(CCarPalDevice, vbatMin), &(mPIDs[20]->mMinValue));


    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_1Max), &(mPIDs[0]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_1Max), &(mPIDs[1]->mMaxValue)); 
    eventPut(aExec, XINDEX(CCarPalDevice, s_trim_2Max), &(mPIDs[2]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, l_trim_2Max), &(mPIDs[3]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, timingMax), &(mPIDs[4]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, rpmMax), &(mPIDs[5]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, spdMax), &(mPIDs[6]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_1Max), &(mPIDs[7]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_2Max), &(mPIDs[8]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_3Max), &(mPIDs[9]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, lam_4Max), &(mPIDs[10]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, mafMax), &(mPIDs[11]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, loadMax), &(mPIDs[12]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, tpsMax), &(mPIDs[13]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, ectMax), &(mPIDs[14]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, fpMax), &(mPIDs[15]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, mapMax), &(mPIDs[16]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, actMax), &(mPIDs[17]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, fuelMax), &(mPIDs[18]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, bapMax), &(mPIDs[19]->mMaxValue));
    eventPut(aExec, XINDEX(CCarPalDevice, vbatMax), &(mPIDs[20]->mMaxValue));

    setFlags(ExecuteOnEventUpdate);
}


bool CCarPalDevice::getData(void)
{
    int res;

    //
    // Check for buffer overflow.
    //
    if (mInputBufferLen >= CARPAL_INPUT_BUFFER_SIZE) {
	DBGFMT_WARN("CarPalDevice::getData(): Input buffer overlow!");
	return false;
    }
    if ((res = read(mDescriptor, mInputBuffer + mInputBufferLen, CARPAL_INPUT_BUFFER_SIZE - mInputBufferLen)) <= 0) {
	DBGFMT_WARN("Failed to read from device: %s", strerror(errno));
	return false;
    }
    mInputBufferLen += res;
    return true;
}

//
// Dump all bytes up to the next block starter.
// Extract aLength and put in aBuffer.
// Return true if successful, 0 if not enough data is
// available.
//
bool CCarPalDevice::extractData(uchar *aBuffer, int aLength)
{
    if (mInputBufferLen < aLength) {
	return false;
    }

    memcpy(aBuffer, mInputBuffer, aLength);
    mInputBufferLen -= (aLength);
    if (mInputBufferLen > 0)
	memmove(mInputBuffer, mInputBuffer + aLength, mInputBufferLen);
#ifdef DEBUG
    {
	int i;
	fprintf(stderr, "Data[%d]:", aLength);
	for (i = 0; i < aLength; i++)
	    fprintf(stderr, " %02X", aBuffer[i]);
	fprintf(stderr, "\n");
    }
#endif
    return true;
}

//
// Read reply
//   <06> <02> <len> d1 ... dlen 
//
bool CCarPalDevice::recvGenericReply(uchar *aBuffer, int* aLength)
{
    uchar hdr[3];
    int len;

    if (mInputBufferLen < 3)
	return false;

    if ((mInputBuffer[0] != 0x06) ||
	(mInputBuffer[1] != REQUEST_GENERIC_COMMAND)) {
	resetData();
	return false;
    }
	
    if ((len = mInputBuffer[2]) < (mInputBufferLen-3))
	return false;  // Expect more

    if (*aLength < len) {
	DBGFMT_CLS("input buffer to small");
	resetData();
	return false;
    }
    extractData(hdr, 3);  // read a skip header
    extractData(aBuffer, len);
    *aLength = len;
}


CCarPalDevice::~CCarPalDevice(void)
{
    DBGFMT_CLS("~CCarPalDevice(): Called");

    if (mDescriptor > -1) 
	close(mDescriptor);

    disconnect(XINDEX(CCarPalDevice,revents));
    m1Release(CFileSource, mSource);

    m1Release(CTimeout, mReadTimeoutTimer);
    m1Release(CTimeout, mPortReopenTimer);
}

bool CCarPalDevice::installPID(CExecutor* aExec, unsigned char aPID)
{
    int i;

    DBGFMT_CLS("supported:  PID[%.2X]", aPID);
    for (i = 0; i < CARPAL_PID_COUNT; i++) {
	if (aPID == mPIDs[i]->mPID) {
	    mProperties[mPIDs[i]->mLevel].push_back(mPIDs[i]);
	    mPIDs[i]->mSupported.putValue(aExec, 1);
	    DBGFMT_CLS("install:  PID[%.2X] Name[%s] Size[%d] PreAdd[%d] Multiplier[%f] PostAdd[%d]",
		       mPIDs[i]->mPID,
		       mPIDs[i]->mName,
		       mPIDs[i]->mSize,
		       mPIDs[i]->mPreAdd,
		       mPIDs[i]->mMultiplier, 
		       mPIDs[i]->mPostAdd);
	    return true;
	}
    }
    return false;
}


//
// Check that this PID is in use by the system.
// If so, then break and request the PID.
// We cannot use the subscriberCount() method here
// since many inactive objects and instruments
// may have a subscription to the PID while not
// themselves being in use.
//

// FIXME!  If we have no supported pids this may crash!
CCarPalDevice::CPID *CCarPalDevice::getNextPID(void)
{
    int i = 0;
    CPID *res = NULL;

    while(1) {
	while(mIterators[i] == mProperties[i].end()) {
	    mIterators[i] = mProperties[i].begin();
	    ++i;
	    if (i == 4) {
		i = 0;
		break;
	    }
	}
	if (((*mIterators[i])->mUsage.value() > 0) || 
	    (mCurrent == mIterators[i])) {
	    res = *mIterators[i];
	    break;
	}
	mIterators[i]++;
    }
    mCurrent = mIterators[i];
    mIterators[i]++;
    return res;
}

bool CCarPalDevice::sendGenericCommand(CExecutor *aExec, uchar cmd, uchar arg)
{
    uchar req[3];

    req[0] = REQUEST_GENERIC_COMMAND;
    req[1] = cmd;
    req[2] = arg;
    DBGFMT_CLS("sendGenericCommand: %02X %02X %02X\n", req[0], req[1], req[2]);
    tcflush(mDescriptor, TCIOFLUSH);
    resetData();
    if (write(mDescriptor, req, 3) != 3) {
	DBGFMT_CLS("sendGenericCommand: Failed to write to CarPal unit [%s]",  strerror(errno));
	return false;
    }
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, reset),  UBool(true));
    return true;
}


bool CCarPalDevice::sendNextRequest(CExecutor *aExec, TimeStamp aTS)
{
    if (!(mCurrentPID = getNextPID()))
	return false;
    if (sendGenericCommand(aExec, 0x01, mCurrentPID->mPID)) {
	mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
	return true;
    }
    return false;
}


CCarPalDevice::CPID *CCarPalDevice::findPID(uchar aPid)
{
    int i;

    for (i = 0; i < CARPAL_PID_COUNT; i++) {
	if (aPid == mPIDs[i]->mPID)
	    return mPIDs[i];
    }
    return 0;
}

void CCarPalDevice::setPID(CExecutor* aExec, uchar *aBuffer, int len)
{
    int val;
    int msize = mCurrentPID->mSize;
    unsigned char pid = mCurrentPID->mPID;

    if (len < (2+msize))
	return;
    if (pid != aBuffer[len-(2+msize)]) {
	DBGFMT_CLS("setPID pid does not match");
	return;
    }

    if (pid == 0x24 || pid == 0x25 ||  // Lam1 or lam2, lam3, lam4
	pid == 0x26 || pid == 0x27 ||
	pid == 0x34 || pid == 0x35 ||
	pid == 0x36 || pid == 0x37) {
	/// mp[0..1] contains the equivalence ratio (0-2)
	// tmp[2..3] contains the voltage (not use)
	// A contains the voltage value.  Not used
	// B contains the percentage value. 
	val = aBuffer[len-2];
    }
    else if (mCurrentPID->mSize == 2)
	val = (aBuffer[len-3] << 8) | aBuffer[len-2];
    else
	val = aBuffer[len-2];

    mCurrentPID->mValue.putValue(aExec,
				 float(mCurrentPID->mPreAdd+val)*
				 mCurrentPID->mMultiplier+
				 float(mCurrentPID->mPostAdd));
}

// Close descriptor and initiate a delayed reopen
void CCarPalDevice::reOpenPort(CExecutor* aExec)
{
    if (mDescriptor > -1) {
	close(mDescriptor);
	mDescriptor = -1;
    }
    mSource->setDescriptor(aExec, -1);
    mState.putValue(aExec, NO_CONNECTION);
    mProtocol.putValue(aExec, UNKNOWN);

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));

    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true));
}


void CCarPalDevice::openPort(CExecutor* aExec, TimeStamp aTimeStamp)
{
    uchar req[16];
    struct termios t;

    // Rst timer.
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));

    if (mDescriptor != -1)
	close(mDescriptor);

    DBGFMT_CLS("openPort(): (Re)opening device [%s] for CarPal communication.", mPort.value().c_str());

    if ((mDescriptor = open(mPort.value().c_str(), O_RDWR|O_NOCTTY)) < 0) {
	DBGFMT_CLS("openPort(): Could not open [%s]: %s", 
		   mPort.value().c_str(), strerror(errno));
	goto fail;
    }
    DBGFMT_CLS("openPort(): mDescriptor=%d", mDescriptor);

    //
    // Setup device
    //
    tcgetattr(mDescriptor, &t);
    t.c_iflag = IGNBRK | IGNPAR;
    t.c_oflag = 0;

    t.c_cflag = CS8 | CLOCAL | CREAD;
    // cfsetospeed(&t, B9600);
    t.c_lflag = 0;
    t.c_cc[VTIME] = 0;
    t.c_cc[VMIN] = 1;
    cfmakeraw(&t);

    if (tcsetattr(mDescriptor, TCSANOW, &t) == -1) {
	DBGFMT_WARN("CarPalDevice:openPort(): Could not set termio on [%s]: %s",  mPort.value().c_str(), strerror(errno));
	goto fail;
    }

    mSource->setDescriptor(aExec, mDescriptor, POLLIN);

    //
    // Flush the buffer
    //
    tcflush(mDescriptor, TCIOFLUSH);

    //
    // Send a request protocol thingy
    //
    req[0] = REQUEST_FOUND_PROTOCOL;
    write(mDescriptor, req, 1);
    mState.putValue(aExec, QUERYING_PROTOCOL);

    // Enable read timeout 
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(6.0));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true));
    return;

 fail:
    DBGFMT_CLS("CCarPalDevice::openPort(): Failed. Will try to reopen later.");
    reOpenPort(aExec);
}


void CCarPalDevice::execute(CExecutor* aExec)
{
    TimeStamp tTimeStamp = aExec->cycleTime();
    uchar req[15];
    uchar res[15];
    int tmp_hex;
    int val, current_val;
    bool changed = false;
    int read_len;
    int frame_count = 0;
    int ind;

    //
    // Port is updated. Close old descriptor and open new one.
    //
    if (mPort.updated()) {
	DBGFMT_CLS("port field updated to [%s]. Will open.", 
		   mPort.value().c_str());
	reOpenPort(aExec);
	return;
    }

    // Process input data if available
    if (mRevents.updated() && (mDescriptor != -1)) {
	getData();
    }

    switch(mState.value()) {
    case UNDEFINED:
	break;

    case NO_CONNECTION:
	if (mPortReopenTimeout.updated() && mPortReopenTimeout.value()) {
	    DBGFMT_CLS("Port reopen timeout. Trying to reopen port.");
	    openPort(aExec, tTimeStamp);
	}
	break;

    case QUERYING_PROTOCOL: {
	uchar res[3];
	u_int16_t proto;
	int i;

	if (!extractData(res, 3)) { // need more?
	    if (mReadTimeout.updated()) {
		goto fail;
	    }
	    return;
	}

	if (res[0] != RESPONSE_FOUND_PROTOCOL) {
	    DBGFMT_CLS("CarPalDevice: (query protocol) bad response %02x\n", 
		       res[0]);
	    goto fail;
	}
	proto = (res[1]<<8) | res[2];
	DBGFMT_CLS("CarPalDevice: (query protocol) protocol=%04x\n", proto);
	i = 0;
	while(mProtocols[i].mProtocol != proto) {
	    if (mProtocols[i].mProtocol == UNKNOWN) {
		goto fail;
	    }
	    i++;
	}
	if (mProtocols[i].mResponseSize == 0) {
	    reOpenPort(aExec);
	    break;
	}

	mUsedProtocol = i;
	mProtocol.putValue(aExec, proto);
	DBGFMT_CLS("(query protocol) device reported protocl %s",
		   mProtocols[i].mName);
	// Start query PIDS
	mPidOffset = 0x00;
	if (!sendGenericCommand(aExec, 0x01, mPidOffset)) {
	    reOpenPort(aExec);
	    break;
	}
	mProperties[0].clear();
	mProperties[1].clear();
	mProperties[2].clear();
	mProperties[3].clear();
	mState.putValue(aExec, QUERYING_PIDS);
	break;
    }

    case QUERYING_PIDS: {
	// int   sz = mProtocols[mUsedProtocol].mResponseSize;
	int sz;
	uchar res[256];
	u_int32_t bits;
	int i;

	sz = sizeof(res);
	if (!recvGenericReply(res, &sz)) {
	    if (mReadTimeout.updated()) {
		goto fail;
	    }
	    return;
	}
	if (sz < 7)
	    return;
	// bits = (res[sz-5] << 24) | (res[sz-4] << 16) | (res[sz-3]<<8)|
	//  (res[sz-2] << 0);
	bits = (res[sz-2]<<24) | (res[sz-3]<<16) | 
	    (res[sz-4]<<8) | (res[sz-5] << 0);
	for (i = 1; i < 32; i++)  {
	    if (bits & (1 << i))
		installPID(aExec, mPidOffset+i);
	}
	if (!(bits & 1)) {
	    int i;
	    // Ok we are done
	    for (i = 0; i < CARPAL_PID_COUNT; i++) {
		if (!mPIDs[i]->mSupported.value())
		    mPIDs[i]->mSupported.putValue(aExec, 0);
	    }
	    // Reset the iterators
	    mIterators[0] = mProperties[0].begin();
	    mIterators[1] = mProperties[1].begin();
	    mIterators[2] = mProperties[2].begin();
	    mIterators[3] = mProperties[3].begin();
	    mCurrent = mIterators[0];
	    
	    mState.putValue(aExec, RUNNING);
	    sendNextRequest(aExec, tTimeStamp);
	}
	else {
	    mPidOffset += 32;
	    sendGenericCommand(aExec, 0x01, mPidOffset);
	}
	break;
    }
	
    case RUNNING: {
	uchar res[256];
	int sz;

	// sz = mProtocols[mUsedProtocol].mResponseSize - 2 + mCurrentPID->mSize;
	sz = sizeof(res);
	if (!recvGenericReply(res, &sz)) {
	    if (mReadTimeout.updated()) {
		goto fail;
	    }
	    return;
	}
	    
	setPID(aExec, res, sz);
	sendNextRequest(aExec, tTimeStamp);
	break;
    }
	
    default:
	goto fail;
    }
    return;

 fail:
    reOpenPort(aExec);
}
