//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "consult_device.hh"
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

XOBJECT_TYPE_BOOTSTRAP(CConsultDevice);

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


CConsultDevice::CConsultDevice(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDescriptor(-1),
    mPort(this),
    mState(this),
    mRevents(this),
    mSource(NULL),
    mLastOpenAttempt(0),
    mPortReopenTimer(NULL),
    mPortReopenTimeout(this),
    mReadTimeoutTimer(NULL),
    mReadTimeout(this),
    mInputBufferLen(0)
{ 
    int i = sizeof(mPIDs)/sizeof(mPIDs[0]);

    mPIDs[0] =  new CPID(aExec, "s_trim_1", this, (uchar) 0xFF, (uchar) 0x1A, 0.0, 0.393, 0.0, 0.0, 100.0 );
    mPIDs[1] =  new CPID(aExec, "l_trim_1", this, (uchar) 0xFF, (uchar) 0x1C, 0.0, 0.393, 0.0, 0.0, 100.0 );
    mPIDs[2] =  new CPID(aExec, "s_trim_2", this, (uchar) 0xFF, (uchar) 0x1B, 0.0, 0.393, 0.0, 0.0, 100.0 );
    mPIDs[3] =  new CPID(aExec, "l_trim_2", this, (uchar) 0xFF, (uchar) 0x1D, 0.0, 0.393, 0.0, 0.0, 100.0 ); 
    mPIDs[4] =  new CPID(aExec, "timing",   this, (uchar) 0xFF, (uchar) 0x16, -110.0, 1.0, 0.0, -110.0, 145.0);
    mPIDs[5] =  new CPID(aExec, "rpm",      this, (uchar) 0x00, (uchar) 0x01, 0.0, 12.5, 0.0, 0, 16383.0 );
    mPIDs[6] =  new CPID(aExec, "spd",      this, (uchar) 0xFF, (uchar) 0x0B, 0.0, 2.0, 0.0, 0.0, 512.0 );
    mPIDs[7] =  new CPID(aExec, "lam_1",    this, (uchar) 0xFF, (uchar) 0x09, 0.0, 0.0001, 0.0, 0.0, 1.275 );
    mPIDs[8] =  new CPID(aExec, "lam_2",    this, (uchar) 0xFF, (uchar) 0x0A, 0.0, 0.0001, 0.0, 0.0, 1.275 );
    mPIDs[9] =  new CPID(aExec, "maf",      this, (uchar) 0x04, (uchar) 0x05, 0.0, 0.05, 0.0, 0.0, 12.0); // gr/sec.
    mPIDs[10] = new CPID(aExec, "load",     this, (uchar) 0xFF, (uchar) 0x33, 0.0, 1.0, 0.0, 0.0, 1.0 );
    mPIDs[11] = new CPID(aExec, "tps",      this, (uchar) 0xFF, (uchar) 0x0D, 0.0, 0.393, 0.0, 9.0, 80.0 );  
    mPIDs[12] = new CPID(aExec, "ect",      this, (uchar) 0xFF, (uchar) 0x08, -50.0, 1.0, 0.0, -50.0, 205.0 );
    mPIDs[13] = new CPID(aExec, "fp",       this, (uchar) 0xFF, (uchar) 0xFF, 0.0, 0.0, 1.0, 0.0, 1.0); // FUTURE
    mPIDs[14] = new CPID(aExec, "map",      this, (uchar) 0xFF, (uchar) 0x29, 0.0, 100, 0.0, 0.0, 1200.0 ); // MUST BE CALIBRATED
    mPIDs[15] = new CPID(aExec, "act",      this, (uchar) 0xFF, (uchar) 0x11, -50.0, 1.0, 0, -50.0, 205.0 );
    mPIDs[16] = new CPID(aExec, "fuel",     this, (uchar) 0xFF, (uchar) 0x2F, 0.0, 0.393, 0.0, 0.0, 100.0 );
    mPIDs[17] = new CPID(aExec, "bap",      this, (uchar) 0xFF, (uchar) 0xFF, 0.0, 1.0, 0.0, 0.0, 1.0 ); // FUTURE
    mPIDs[18] = new CPID(aExec, "vbat",     this, (uchar) 0xFF, (uchar) 0x0C, 0.0, 0.08, 0.0, 0.0, 20.4);
    mPIDs[19] = new CPID(aExec, "fpw",      this, (uchar) 0x14, (uchar) 0x15, 0.0, 0.01, 0.0, 0.0, 20.0);

    eventPut(aExec, XINDEX(CConsultDevice, revents), &mRevents);  // File revents trigger event.

    eventPut(aExec, XINDEX(CConsultDevice, port), &mPort);
    mState.putValue(aExec, NO_CONNECTION);
    eventPut(aExec, XINDEX(CConsultDevice, state), &mState);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);
    connect(XINDEX(CConsultDevice,revents), mSource, XINDEX(CFileSource,revents));

    //
    // Create port reopen timer.
    //
    //    mPortReopenTimer = m1New(CTimeout, aExec);
    mPortReopenTimer = (CTimeout *) CTimeout::CTimeoutType::singleton()->produce(aExec, NULL, NULL).o;
    m1Retain(CTimer, mPortReopenTimer);
    aExec->addComponent(mPortReopenTimer);

    mPortReopenTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(1.05));

    eventPut(aExec, XINDEX(CConsultDevice, port_reopen_timeout), &mPortReopenTimeout);
    connect(XINDEX(CConsultDevice, port_reopen_timeout), mPortReopenTimer, XINDEX(CTimeout, timeout));

    //
    // Create read timeout timer.
    //
    mReadTimeoutTimer = m1New(CTimeout, aExec);
    m1Retain(CTimeout, mReadTimeoutTimer);
    aExec->addComponent(mReadTimeoutTimer);

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(0.5));

    eventPut(aExec, XINDEX(CConsultDevice, read_timeout), &mReadTimeout);
    connect(XINDEX(CConsultDevice, read_timeout), mReadTimeoutTimer, XINDEX(CTimeout, timeout));

    while(i--) {
	mPIDs[i]->mValue.putValue(aExec, mPIDs[i]->mMinValue.value());
	mPIDs[i]->mUsage.putValue(aExec, 0);
    }

    eventPut(aExec, XINDEX(CConsultDevice, s_trim_1), &(mPIDs[0]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_1), &(mPIDs[1]->mValue)); 
    eventPut(aExec, XINDEX(CConsultDevice, s_trim_2), &(mPIDs[2]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_2), &(mPIDs[3]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, timing), &(mPIDs[4]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, rpm), &(mPIDs[5]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, spd), &(mPIDs[6]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, lam_1), &(mPIDs[7]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, lam_2), &(mPIDs[8]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, maf), &(mPIDs[9]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, load), &(mPIDs[10]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, tps), &(mPIDs[11]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, ect), &(mPIDs[12]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, fp), &(mPIDs[13]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, map), &(mPIDs[14]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, act), &(mPIDs[15]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, fuel), &(mPIDs[16]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, bap), &(mPIDs[17]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, vbat), &(mPIDs[18]->mValue));
    eventPut(aExec, XINDEX(CConsultDevice, fpw), &(mPIDs[19]->mValue));

    eventPut(aExec, XINDEX(CConsultDevice, s_trim_1Supported), &(mPIDs[0]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_1Supported), &(mPIDs[1]->mSupported)); 
    eventPut(aExec, XINDEX(CConsultDevice, s_trim_2Supported), &(mPIDs[2]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_2Supported), &(mPIDs[3]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, timingSupported), &(mPIDs[4]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, rpmSupported), &(mPIDs[5]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, spdSupported), &(mPIDs[6]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, lam_1Supported), &(mPIDs[7]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, lam_2Supported), &(mPIDs[8]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, mafSupported), &(mPIDs[9]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, loadSupported), &(mPIDs[10]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, tpsSupported), &(mPIDs[11]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, ectSupported), &(mPIDs[12]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, fpSupported), &(mPIDs[13]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, mapSupported), &(mPIDs[14]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, actSupported), &(mPIDs[15]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, fuelSupported), &(mPIDs[16]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, bapSupported), &(mPIDs[17]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, vbatSupported), &(mPIDs[18]->mSupported));
    eventPut(aExec, XINDEX(CConsultDevice, fpwSupported), &(mPIDs[19]->mSupported));

    eventPut(aExec, XINDEX(CConsultDevice, s_trim_1Usage), &(mPIDs[0]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_1Usage), &(mPIDs[1]->mUsage)); 
    eventPut(aExec, XINDEX(CConsultDevice, s_trim_2Usage), &(mPIDs[2]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_2Usage), &(mPIDs[3]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, timingUsage), &(mPIDs[4]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, rpmUsage), &(mPIDs[5]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, spdUsage), &(mPIDs[6]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, lam_1Usage), &(mPIDs[7]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, lam_2Usage), &(mPIDs[8]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, mafUsage), &(mPIDs[9]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, loadUsage), &(mPIDs[10]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, tpsUsage), &(mPIDs[11]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, ectUsage), &(mPIDs[12]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, fpUsage), &(mPIDs[13]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, mapUsage), &(mPIDs[14]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, actUsage), &(mPIDs[15]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, fuelUsage), &(mPIDs[16]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, bapUsage), &(mPIDs[17]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, vbatUsage), &(mPIDs[18]->mUsage));
    eventPut(aExec, XINDEX(CConsultDevice, fpwUsage), &(mPIDs[19]->mUsage));

    eventPut(aExec, XINDEX(CConsultDevice, s_trim_1Min), &(mPIDs[0]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_1Min), &(mPIDs[1]->mMinValue)); 
    eventPut(aExec, XINDEX(CConsultDevice, s_trim_2Min), &(mPIDs[2]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_2Min), &(mPIDs[3]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, timingMin), &(mPIDs[4]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, rpmMin), &(mPIDs[5]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, spdMin), &(mPIDs[6]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, lam_1Min), &(mPIDs[7]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, lam_2Min), &(mPIDs[8]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, mafMin), &(mPIDs[9]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, loadMin), &(mPIDs[10]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, tpsMin), &(mPIDs[11]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, ectMin), &(mPIDs[12]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, fpMin), &(mPIDs[13]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, mapMin), &(mPIDs[14]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, actMin), &(mPIDs[15]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, fuelMin), &(mPIDs[16]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, bapMin), &(mPIDs[17]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, vbatMin), &(mPIDs[18]->mMinValue));
    eventPut(aExec, XINDEX(CConsultDevice, fpwMin), &(mPIDs[19]->mMinValue));

    eventPut(aExec, XINDEX(CConsultDevice, s_trim_1Max), &(mPIDs[0]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_1Max), &(mPIDs[1]->mMaxValue)); 
    eventPut(aExec, XINDEX(CConsultDevice, s_trim_2Max), &(mPIDs[2]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, l_trim_2Max), &(mPIDs[3]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, timingMax), &(mPIDs[4]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, rpmMax), &(mPIDs[5]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, spdMax), &(mPIDs[6]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, lam_1Max), &(mPIDs[7]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, lam_2Max), &(mPIDs[8]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, mafMax), &(mPIDs[9]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, loadMax), &(mPIDs[10]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, tpsMax), &(mPIDs[11]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, ectMax), &(mPIDs[12]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, fpMax), &(mPIDs[13]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, mapMax), &(mPIDs[14]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, actMax), &(mPIDs[15]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, fuelMax), &(mPIDs[16]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, bapMax), &(mPIDs[17]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, vbatMax), &(mPIDs[18]->mMaxValue));
    eventPut(aExec, XINDEX(CConsultDevice, fpwMax), &(mPIDs[19]->mMaxValue));

    setFlags(ExecuteOnEventUpdate);
}

bool CConsultDevice::getData(void)
{
    int res;

    //
    // Check for buffer overflow.
    //
    if (mInputBufferLen >= CONSULT_INPUT_BUFFER_SIZE) {
	DBGFMT_WARN("CConsultDevice::getData(): Input buffer overlow!");
	return false;
    }
    if ((res = read(mDescriptor, mInputBuffer + mInputBufferLen, CONSULT_INPUT_BUFFER_SIZE - mInputBufferLen)) <= 0) {
	DBGFMT_WARN("Failed to read from device: %s", strerror(errno));
	return false;
    }
    mInputBufferLen += res;

    //    DBGFMT_CLS("CConsultDevice::getData(): Got %d bytes. Total len %d bytes", res, mInputBufferLen);

    return true;
}

/* timeout is in msec */
int CConsultDevice::readData(uchar *aBuffer, int aLength, int mTimeout)
{
    struct pollfd pfd;
    int res;

    //
    // First check if we already have enough data preread.
    //
    if (mInputBufferLen >= aLength) {
	memcpy(aBuffer, mInputBuffer, aLength);
	mInputBufferLen -= aLength;
	if (mInputBufferLen > 0) 
	    memmove(mInputBuffer, mInputBuffer + aLength, mInputBufferLen);
	return aLength;
    }

    pfd.fd = mDescriptor;
    pfd.events = POLLIN;
    pfd.revents = 0;
    
    // Fill up existing buffer until we have the desired number of bytes.
    while(mInputBufferLen < aLength) {
	if (m1Poll(&pfd, 1, mTimeout) == 0) {
	    DBGFMT_CLS("Timeout while reading [%d] msec.", mTimeout);
	    // Copy what we have.
	    res = mInputBufferLen;
	    memcpy(aBuffer, mInputBuffer, mInputBufferLen);
	    mInputBufferLen = 0;
	    return res;
	}	    

	if (pfd.revents != POLLIN) // Not input
	    continue;

	if ((res = read(mDescriptor, mInputBuffer + mInputBufferLen, CONSULT_INPUT_BUFFER_SIZE - mInputBufferLen)) <= 0) {
	    DBGFMT_WARN("Failed to read from device: %s", strerror(errno));
	    return -1;
	}
	mInputBufferLen += res;
    }
    // Copy what we need.
    memcpy(aBuffer, mInputBuffer, aLength);
    mInputBufferLen -= aLength;
    if (mInputBufferLen > 0) 
	memmove(mInputBuffer, mInputBuffer + aLength, mInputBufferLen);
    return aLength;
}

//
// Do a read from the available input buffer only.
// If not enough data is available to satisfy the read,
// return 0.
//
bool CConsultDevice::extractData(uchar *aBuffer, int aLength)
{

    //
    // First check if we already have enough data preread.
    //
    if (mInputBufferLen < aLength) {
	//	DBGFMT_CLS("CConsultDevice::extractData(): Wanted[%d] bytes. Have[%d]", aLength, mInputBufferLen);
	return false;
    }
    //    DBGFMT_CLS("CConsultDevice::extractData(): Extracted[%d] bytes.", aLength);
    memcpy(aBuffer, mInputBuffer, aLength);
    mInputBufferLen -= aLength;
    if (mInputBufferLen > 0) 
	memmove(mInputBuffer, mInputBuffer + aLength, mInputBufferLen);
    return true;
}
    
CConsultDevice::~CConsultDevice(void)
{
    DBGFMT_CLS("CConsultDevice::~CConsultDevice(): Called");

    if (mDescriptor > -1) 
	close(mDescriptor);

    disconnect(XINDEX(CConsultDevice,revents));
    m1Release(CFileSource, mSource);

    m1Release(CTimeout, mReadTimeoutTimer);
    m1Release(CTimeout, mPortReopenTimer);
}


bool CConsultDevice::stopStreaming(void)
{
    uchar buf;
    if (mDescriptor == -1) {
	DBGFMT_CLS("CConsultDevice::stopStreaming(): No device open");
	return false;
    }
    //    DBGFMT_CLS("Stream stop");
    write(mDescriptor, "\x30", 1);
    while(readData(&buf, 1, 50) == 1 && buf != 0xCF)
	//	DBGFMT_CLS("CConsultDevice::stopStreaming(): Bleeding off[0x%.2X]\n", buf & 0xFF);
	; // Bleed of residual data.
}

//
// Extract all supported PIDs
//
bool CConsultDevice::getSupportedPIDs(CExecutor* aExec)
{
    int ind;
    DBGFMT_CLS("CConsultDevice::GetSupportedPIDs(): Locating supported PIDs.");

    // Stop old streaming data.
    stopStreaming();

    // Clear old list.
    mActivePIDs.clear();

    for (ind = 0; ind < CONSULT_PID_COUNT; ++ind) {
	uchar buf[3];
	uchar rsp[3];
	int res;
	
	//
	// Check if this PID is future marked in the master list
	//
	if (mPIDs[ind]->mMSBReg == 0xFF && mPIDs[ind]->mLSBReg == 0xFF)
	    continue;
	    
	buf[0] = 0x5A;
	buf[1] = mPIDs[ind]->mLSBReg;
	buf[2] = 0xF0;

	
	//	DBGFMT_CLS("CConsultDevice::getSupportedPIDs(): Checking[0x%.2X]", mPIDs[ind]->mLSBReg);
	write(mDescriptor, buf, 3);

	if ((res = readData(rsp, 3, 100)) != 3) {
	    DBGFMT_CLS("getSupportedPIDs(): Short read [%d]", res);
	    return false;
	}

	if (rsp[0] == 0xA5 && rsp[1] == buf[1] && rsp[2] == 0xFF) {
	    DBGFMT_CLS("CConsultDevice::getSupportedPIDs(): [%s] supported", mPIDs[ind]->mName);
	    mActivePIDs.push_back(mPIDs[ind]);
	    mPIDs[ind]->mSupported.putValue(aExec, 1);
	} // else
// 	    DBGFMT_CLS("CConsultDevice::getSupportedPIDs(): [%s] not supported rsp 0x%.2X 0x%.2X 0x%.2X", 
// 		       mPIDs[ind]->mName, rsp[0], rsp[1], rsp[2]);

	// Stop streaming

	stopStreaming();
    }
    return true;
}
 
bool CConsultDevice::setupStreamingProfile(void)
{
    int len = 0;
    uchar cmd[128];
    uchar rsp[128];
    int res;
    
    stopStreaming();
    mStreamingPIDs.clear();
    DBGFMT_CLS("CConsultDevice::setupStreamingProfile(): ActivePid count[%d]", mActivePIDs.size());

    // Go through all active PIDs and add those with a usage greater than 0 and build up the command.
    for(CPIDListIterator iter = mActivePIDs.begin(); iter != mActivePIDs.end(); ++iter) {
	//
	// Skip active pids currently not used by the M1.
	//
	DBGFMT_CLS("CConsultDevice::setupStreamingProfile(): Checking [%s] usage[%d]", (*iter)->mName, (*iter)->mUsage.value());
	if (!(*iter)->mUsage.value()) 
	    continue;

	cmd[len++] = 0x5A;
	cmd[len++] = (*iter)->mLSBReg;
	// Add high byte if 16 bit val.
	if ((*iter)->mMSBReg != 0xFF) {
	    cmd[len++] = 0x5A;
	    cmd[len++] = (*iter)->mMSBReg;
	}
	// Add to streaming pids
	DBGFMT_CLS("CConsultDevice::setupStreamingProfile(): Adding [%s] usage[%d]", (*iter)->mName, (*iter)->mUsage.value());
	mStreamingPIDs.push_back(*iter);
    }
    cmd[len++] = 0xF0;
//     for(res = 0; res < len; ++res) 
// 	DBGFMT_CLS("CConsultDevice::setupStreamingProfile(): Strm[%d][0x%.02X]", res, cmd[res]);
	
    write(mDescriptor, cmd, len);

    if ((res = readData(rsp, len, 100)) != len) {
	DBGFMT_WARN("CConsultDevice::setupStreamingProfile(): Short response.  %d bytes, got %d", len, res);
	return false;
    }
//     for(res = 0; res < len; ++res) 
// 	DBGFMT_CLS("CConsultDevice::setupStreamingProfile(): Strm[%d][0x%.02X]", res, rsp[res]);
    
    return true;
}


bool CConsultDevice::processStream(CExecutor* aExec) 
{
    uchar buf[512];
    CPIDListIterator iter;
    uchar *current = buf;
    int len = 1; // Initial count.
    int res;

    if (mStreamingPIDs.size() == 0) {
	DBGFMT_CLS("CConsultDevice::processStream(): No streaming PIDs are expected.");
	return false;
    }
    
    for(iter = mStreamingPIDs.begin(); iter != mStreamingPIDs.end(); ++iter) {
	// Check if we should process two bytes
	if ((*iter)->mMSBReg != 0xFF) 
	    len += 2;
	else 
	    len += 1;
    }
    len++; // Terminating 0xFF.


    //
    // Data should be readily available since we triggered on poll.
    //
    getData();

    //
    // Rst timeout.
    //
    while (extractData(buf, len)) {
	current = buf;
	//
	// Validate length.
	//
	if (buf[0] != len - 2) {
	    DBGFMT_CLS("CConsultDevice::processStream(): Packet header indicates [%d] PIDs, but I want [%d]", buf[0], len - 2);
	    return false;
	}

	//
	// Check how much we should read.
	//
	if (buf[len - 1] != 0xFF) {
	    DBGFMT_CLS("CConsultDevice::processStream(): Last byte is not 0xFF, but 0x%.2X", buf[len - 1]);
	    return false;
	}
    
	//
	// The items in the buffer are listed in the same order as the items in the mStreamingPID list.
	//
	current++;
	for(iter = mStreamingPIDs.begin(); iter != mStreamingPIDs.end(); ++iter) {
	    int val;
	    CPID *pid = *iter;

	    // Check if we should process two bytes
	    if (pid->mMSBReg != 0xFF) {
		val = *current | (*(current + 1) << 8);
		current += 2;
	    } else {
		val = *current;
		current++;
	    }
	
	    pid->mValue.putValue(aExec, pid->mPreAdd + val * pid->mMultiplier + pid->mPostAdd);
	    //	    DBGFMT_CLS("CConsultDevice::processStream(): PID[%s] set to [%d]/%f]", pid->mName, val, pid->mValue.value());
	}
    }
    return true;
}

void CConsultDevice::openPort(CExecutor* aExec, TimeStamp aTimeStamp)
{
    uchar res[100];
    struct termios t;

    // Rst timer.
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false), TRIGGER_YES);

    if (mDescriptor != -1)
	close(mDescriptor);

    DBGFMT_CLS("CConsultDevice::openPort(): (Re)opening device [%s] for Consult communication.", mPort.value().c_str());
#if defined(O_NDELAY) && defined(F_SETFL)
    if ((mDescriptor=open(mPort.value().c_str(),O_RDWR|O_NDELAY|O_NOCTTY))<0) {
	DBGFMT_CLS("CConsultDevice:openPort(): Could not open [%s]: %s", 
		   mPort.value().c_str(), strerror(errno));
	goto fail;
    }
    // Now clear O_NDELAY
    // fl = fcntl(mDescriptor, F_GETFL, 0);
    // fcntl(mDescriptor, F_SETFL, fl & ~O_NDELAY);
#else
    if ((mDescriptor = open(mPort.value().c_str(), O_RDWR|O_NOCTTY)) < 0) {
	DBGFMT_CLS("CConsultDevice:openPort(): Could not open [%s]: %s", 
		   mPort.value().c_str(), strerror(errno));
	goto fail;	
    }
#endif
    DBGFMT_CLS("CConsultDevice:openPort(): mDescriptor=%d", mDescriptor);

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
    t.c_cc[VMIN] = 1;
    cfmakeraw(&t);
 
    if (tcsetattr(mDescriptor, TCSANOW, &t) == -1) {
	DBGFMT_WARN("CConsultDevice:openPort(): Could not set termio on [%s]: %s",  mPort.value().c_str(), strerror(errno));
	goto fail;
    }

    mSource->setDescriptor(aExec, mDescriptor, POLLIN);

    DBGFMT_CLS("CConsultDevice:openPort(): setDescripor:%d", mDescriptor);

    //
    // Init ECU comm
    // 
    if (write(mDescriptor, "\xff\xff\xef", 3) == -1) {
	DBGFMT_WARN("Could not send init command to ECU: %s", strerror(errno));
	goto fail;
    }

    DBGFMT_CLS("CConsultDevice:openPort(): readResponse:%d", mDescriptor);
    
    // Bleed off response.
    readData(res, 10, 50) ;


    // Get supported protocols
    if (!getSupportedPIDs(aExec))
	goto fail;

    setupStreamingProfile();
    DBGFMT_CLS("CConsultDevice::openPort(): Done.");
    mState.putValue(aExec, RUNNING);
    return;

 fail:
    DBGFMT_CLS("CConsultDevice::openPort(): Failed. Will try to reopen later.");
    mState.putValue(aExec, NO_CONNECTION);
    if (mDescriptor != -1) {
	close(mDescriptor);
	mDescriptor = -1;
	mSource->setDescriptor(aExec, -1);
    }

    //
    // Setup a reopen time
    //
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false), TRIGGER_YES);
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true), TRIGGER_YES);
    return;
}

void CConsultDevice::execute(CExecutor* aExec)
{
    TimeStamp tTimeStamp = aExec->cycleTime();
    CPIDListIterator iter;
	
    //
    // Port is updated. Close old descriptor and open new one.
    //
    
    //    DBGFMT_CLS("Updated[%d] timeout[%d].", mPortReopenTimeout.updated()?1:0 , (mPortReopenTimeout == true)?1:0);

    if (mPort.updated() || (mPortReopenTimeout.updated() && mPortReopenTimeout.value() == true)) {
	DBGFMT_CLS("port field updated to [%s]. Will open.", mPort.value().c_str());
	openPort(aExec, tTimeStamp);
    }

     if (mDescriptor == -1)
	return;
    
     if (mReadTimeout.updated() && mReadTimeout.value() == true) {
	DBGFMT_CLS("Lost communication. Trying to reopen port.");
	goto fail;
	return;
    }	
    
    //
    // Check if usage is updated.
    //
    
    for(iter = mActivePIDs.begin(); 
	iter != mActivePIDs.end() && !(*iter)->mUsage.updated();    
	++iter) {
	//	DBGFMT_CLS("%s usage[%d] Updated[%d]", (*iter)->mName, (*iter)->mUsage.self(), (*iter)->mUsage.updated());
    }
    if (iter != mActivePIDs.end()) {
	setupStreamingProfile();
	return;
    }
	
    
    if (!mRevents.updated()) {
	DBGFMT_CLS("No input.");
	return;
    }	

    if (!processStream(aExec))
	goto fail;

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true), TRIGGER_YES);
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);

    return;

 fail:
    DBGFMT_CLS("FAIL\n");
    mState.putValue(aExec, NO_CONNECTION);
    if (mDescriptor != -1) {
	close(mDescriptor);
	mDescriptor = -1;
	mSource->setDescriptor(aExec, -1);
    }


    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false), TRIGGER_YES);

    mPortReopenTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(true), TRIGGER_YES);
    mPortReopenTimer->put(aExec, XINDEX(CTimeout, reset), UBool(true), TRIGGER_YES);
    return;
}


