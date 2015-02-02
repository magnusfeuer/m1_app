//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
// YDWPW8RZ4S
//
#include "lc1_device.hh"
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

XOBJECT_TYPE_BOOTSTRAP(CLC1Device);

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


CLC1Device::CLC1Device(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDescriptor(-1),
    mPort(this),
    mState(this),
    mRevents(this),
    mSource(NULL),
    mReadTimeoutTimer(NULL),
    mReadTimeout(this),
    mInputBufferLen(0),
    mFirstRead(false),
    mLam(this)
{ 
    eventPut(aExec, XINDEX(CLC1Device, revents), &mRevents);  // File revents trigger event.
    eventPut(aExec, XINDEX(CLC1Device, port), &mPort);
    mState.putValue(aExec, NO_CONNECTION);
    eventPut(aExec, XINDEX(CLC1Device, state), &mState);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);
    connect(XINDEX(CLC1Device,revents), mSource, XINDEX(CFileSource,revents));


    //
    // Create read timeout timer.
    //
    //    mReadTimeoutTimer = m1New(CTimeout, aExec);
    mReadTimeoutTimer = (CTimeout *) CTimeout::CTimeoutType::singleton()->produce(aExec, NULL, NULL).o;
    m1Retain(CTimeout, mReadTimeoutTimer);
    aExec->addComponent(mReadTimeoutTimer);

    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, enabled), UBool(false));
    mReadTimeoutTimer->put(aExec, XINDEX(CTimeout, duration), UFloat(0.5));

    eventPut(aExec, XINDEX(CLC1Device, read_timeout), &mReadTimeout);
    connect(XINDEX(CLC1Device, read_timeout), mReadTimeoutTimer, XINDEX(CTimeout, timeout));

    //
    // Setup channel.
    //
    mLam.putValue(aExec, 1.0);
    eventPut(aExec, XINDEX(CLC1Device, lam), &mLam);
    setFlags(ExecuteOnEventUpdate);
}

bool CLC1Device::getData(void)
{
    int res;

    //
    // Check for buffer overflow.
    //
    if (mInputBufferLen >= LC1_INPUT_BUFFER_SIZE) {
	DBGFMT_WARN("CLC1Device::getData(): Input buffer overlow!");
	return false;
    }
    if ((res = read(mDescriptor, mInputBuffer + mInputBufferLen, LC1_INPUT_BUFFER_SIZE - mInputBufferLen)) <= 0) {
	DBGFMT_WARN("Failed to read from device: %s", strerror(errno));
	return false;
    }
    mInputBufferLen += res;

    //    DBGFMT_CLS("CLC1Device::getData(): Got %d bytes. Total len %d bytes", res, mInputBufferLen);

    return true;
}


//
// Dump all bytes up to the next block starter.
// Extract 14 bytes and put in aBuffer.
// Return true if successful, 0 if not enough data is
// available.
//
int CLC1Device::extractData(uchar *aBuffer, int aMaxLength)
{
    int start_ind = 0;
    int end_ind = 0;

    if (aMaxLength < 1)
	return 0;

    //
    // Skip until we have sync byte.
    //
    while(mInputBufferLen - start_ind > 0 &&
	  (mInputBuffer[start_ind] & 0x80) == 0x00) {
	DBGFMT_CLS("buf[%d]=0x%.2X", start_ind, mInputBuffer[start_ind] & 0xFF);
	++start_ind;
    }
    

    //
    // No sync found in entire buffer
    //
    if (mInputBufferLen - start_ind == 0) {
	mInputBufferLen = 0;
	return 0;
    }

    end_ind = start_ind + 1;

    //
    // Copy bytes until we see another sync byte.
    // Stupid protocol with non-determenistic length.
    //
    while(mInputBufferLen - end_ind > 0 &&  // We still have data in input buffer
	  aMaxLength - end_ind > 0 &&   // We still have space in aBuffer
	  (mInputBuffer[end_ind] & 0x80) == 0x00)  // We are not on a new sync byte.
	end_ind++;

    //
    // Check if we hit end of input buffer and we still want more data.
    // If so, we do not have enough data to satisfy call requirements.
    // Just return
    //
    if (mInputBuffer - end_ind == 0 && end_ind - start_ind < aMaxLength)
	return 0;

    // Copy as much as we can.
    memcpy(aBuffer, mInputBuffer + start_ind, end_ind - start_ind);

    // Move remaining of buffer to beginning.
    memmove(mInputBuffer, mInputBuffer + end_ind, mInputBufferLen - end_ind);
    mInputBufferLen -= end_ind;
    return end_ind - start_ind;
}
    
CLC1Device::~CLC1Device(void)
{
    DBGFMT_CLS("CLC1Device::~CLC1Device(): Called");

    if (mDescriptor > -1) 
	close(mDescriptor);

    disconnect(XINDEX(CLC1Device,revents));
    m1Release(CFileSource, mSource);

    m1Release(CTimeout, mReadTimeoutTimer);
}


bool CLC1Device::processStream(CExecutor* aExec) 
{
    uchar buf[16];
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
    getData();

    //
    // Read data.
    //
    while (extractData(buf, 16)) {
	int multiplier;
	// If status word is not cleared, just return
	if ((buf[0] & 0x7C) != 0x00)
	    return true;

	// Bit massage the AFR multiplier. Not used today.
	multiplier = ((buf[0] & 0x01) << 7) | (buf[1] & 0x7F);

	// Bit massage the lambda value
	mLam.putValue(aExec, 0.5 + float(((buf[2] & 0x3F) << 7) | buf[3] & 0x7F));
    }
    return true;
}

void CLC1Device::openPort(CExecutor* aExec, TimeStamp aTimeStamp)
{
    uchar res[100];
    struct termios t;

    if (mDescriptor != -1)
	close(mDescriptor);

    DBGFMT_CLS("CLC1Device::openPort(): (Re)opening device [%s] for Consult communication.", mPort.value().c_str());

    if ((mDescriptor = open(mPort.value().c_str(), O_RDWR|O_NOCTTY)) < 0) {
	DBGFMT_CLS("CLC1Device:openPort(): Could not open [%s]: %s", 
		   mPort.value().c_str(), strerror(errno));
	goto fail;	
    }
    DBGFMT_CLS("CLC1Device:openPort(): mDescriptor=%d", mDescriptor);

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
	DBGFMT_WARN("CLC1Device:openPort(): Could not set termio on [%s]: %s",  mPort.value().c_str(), strerror(errno));
	goto fail;
    }

    mSource->setDescriptor(aExec, mDescriptor, POLLIN);
    DBGFMT_CLS("CLC1Device::openPort(): Done.");
    mState.putValue(aExec, RUNNING);
    mFirstRead = true;
    return;

 fail:
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
    mFirstRead = false;
    return;
}

void CLC1Device::execute(CExecutor* aExec)
{
    TimeStamp tTimeStamp = aExec->cycleTime();
	
    //
    // Port is updated. Close old descriptor and open new one.
    //
    

    if (mPort.updated()) {
	DBGFMT_CLS("port field updated to [%s]. Will open.", mPort.value().c_str());
	openPort(aExec, tTimeStamp);
    }

     if (mDescriptor == -1)
	return;
    
     if (mReadTimeout.updated() && mReadTimeout.value() == true) {
	DBGFMT_CLS("Lost communication.");
	goto fail;
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
    mFirstRead = false;
    return;
}


