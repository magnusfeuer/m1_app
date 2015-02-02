//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2008.
//

// 
// Magden propreitary AD converter protocol
//
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>


#include "ad_dev.hh"

XOBJECT_TYPE_BOOTSTRAP(CADDevice);

#define RESPONSE_OK    13
#define RESPONSE_ERROR 7
#define RESPONSE_tOK   'z'
#define RESPONSE_TOK   'Z'

static int adWrite(int fd, char* data)
{
    return write(fd, data, strlen(data));
}

static unsigned int getHex(char *aInput, int aLen) 
{
    char buf[32];
    if (aLen > 31)
	return 0;

    strncpy(buf, aInput, aLen);
    buf[aLen] = 0; // strncpy doesn't guarantee null term.
    
    return (unsigned int) strtol(buf, 0, 16);
}


CADDevice::CADDevice(CExecutor* aExec, CBaseType *aType) :
    CExecutable(aExec, aType),
    mPort(this),
    mPortSpeed(this),
    mWait(WAIT_NONE),
    mDescriptor(-1),
    mSimulation(false),
    mRevents(this),
    mState(this),
    mSource(NULL),
    mTimeoutObj(0),
    mTimeout(this),
    mInputBufferLen(0)
{ 
    CArray *a;
    CArrayType* t;
 
    mDescriptor = -1;
    mPort.putValue(aExec, "");
    mPortSpeed.putValue(aExec, 0);
    mRevents.putValue(aExec, 0);
    mState.putValue(aExec,AD_DEVICE_STATE_NONE);
    mTimeout.putValue(aExec, false);
    eventPut(aExec,XINDEX(CADDevice, port), &mPort);
    eventPut(aExec,XINDEX(CADDevice, portSpeed), &mPortSpeed);
    eventPut(aExec,XINDEX(CADDevice, revents), &mRevents);
    eventPut(aExec,XINDEX(CADDevice, timeout), &mTimeout);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);
    connect(XINDEX(CADDevice,revents), 
	    mSource, XINDEX(CFileSource,revents));

    // Install array.
    t = CArrayType::create(CADChannel::CADChannelType::singleton(), 16);
    a = new CArray(aExec, t, sizeof(CADChannel *), 16); // Array of channels
    put(aExec, XINDEX(CADDevice,channels), UArray(a));
    setFlags(ExecuteOnEventUpdate);

    //
    // Go through array elements and init them
    //
    for(int i=0; i < 16; ++i) {
	CADChannel *chan =  m1New(CADChannel, aExec, CADChannel::CADChannelType::singleton());

	chan->setup(this, i);
	a->put(aExec, i, UObject(chan));
    }

    //
    // Create a timeout. Will be enabled during port reopen attempts.
    //
    mTimeoutObj = (CTimeout *) CTimeout::CTimeoutType::singleton()->produce(aExec, NULL, NULL).o;
    m1Retain(CTimer, mTimeoutObj);
    aExec->addComponent(mTimeoutObj);
    mTimeoutObj->put(aExec, XINDEX(CTimeout, autoDisconnect), UBool(false));
    mTimeoutObj->put(aExec, XINDEX(CTimeout, enabled), UFalse());
    mTimeoutObj->put(aExec, XINDEX(CTimeout, duration), UFloat(1.00));

    // Connect CTimeout.timeout to local timeout event bool.
    connect(XINDEX(CADDevice, timeout), mTimeoutObj, XINDEX(CTimeout, timeout));
}



CADDevice::~CADDevice()
{
    if (mDescriptor != -1) {
	// FIXME: send close channel
	close(mDescriptor);
    }
    m1Release(CFileSource, mSource);

    disconnect(XINDEX(CADDevice,revents));
    disconnect(XINDEX(CADDevice,timeout));
    m1Release(CTimeout, mTimeoutObj);
}


//
// Read a lot of data and store it.
//
int CADDevice::readData(void)
{
    int res;

    if (mInputBufferLen >= AD_INPUT_BUFFER_SIZE) {
	DBGFMT_WARN("CADDevice: Buffer overflow!");
	return -1;
    }
	
    if ((res = read(mDescriptor, mInputBuffer + mInputBufferLen, AD_INPUT_BUFFER_SIZE - mInputBufferLen)) <= 0) {
	DBGFMT_WARN("Failed to read from device: %s", strerror(errno));
	return -1;
    }
    mInputBufferLen += res;
    mInputBuffer[mInputBufferLen] = 0;
    return res;
}

//
// Do a read from the available input buffer only.
// If not enough data is available to satisfy the read,
// return 0.
//
int CADDevice::getLine(char *aBuffer, int aMaxLength)
{
    char *nl = strchr(mInputBuffer, '\n');
    int l_sz;
    int c_sz;

    //
    // No newline?
    //
    if (!nl)  
	return 0;


    l_sz = nl - mInputBuffer; // Line size
    c_sz = (aMaxLength < l_sz)?aMaxLength:l_sz; // Copy size

    // Copy line from buffer to target.
    if (c_sz > 0) 
	memcpy(aBuffer, mInputBuffer, c_sz);
	
    aBuffer[c_sz] = 0;
    mInputBufferLen -= (c_sz + 1); // Kill nl.

    // Move remaining buffer to beginning.
    if (mInputBufferLen > 0) 
	memmove(mInputBuffer, mInputBuffer + c_sz + 1, mInputBufferLen);

    mInputBuffer[mInputBufferLen] = 0;

    return c_sz;
}


void CADDevice::start(CExecutor* aExec)
{
    DBGFMT_CLS("CADDevice::start()");
}


void CADDevice::setSampleInterval(unsigned int aChannelIndex, unsigned int aSampleInterval)
{
    char buf[32];

    if (aSampleInterval > 0x0400)
	aSampleInterval = 0x0400;

    // If nothing is open, wait with setting up sampling until later.
    if (mDescriptor == -1) 
	return;

    // Turn off sampling?
    if(aSampleInterval == 0) {
	DBGFMT_CLS("CADDevice::setSampleInterval(): Turning off sampling for channel[%.1X]", aChannelIndex);
	sprintf(buf, "A%.2X0000\n", aChannelIndex);
	adWrite(mDescriptor, buf);
	return;
    }

    // Setup sampling
    DBGFMT_CLS("CADDevice::setSampleInterval(): Enabling sampling for channel[%.1X] every [%d] msec", 
	       aChannelIndex, aSampleInterval);

    sprintf(buf, "A%.2X%.4X\na%.2X\n", aChannelIndex, aSampleInterval, aChannelIndex);

    adWrite(mDescriptor, buf);
    return;
}


void CADDevice::execute(CExecutor* aExec)
{
    // Port to open?
    if (mPort.updated() || mPortSpeed.updated()) {
	closeDevice(aExec);
	openDevice(aExec);
    }


    // Data to read?
    if (mDescriptor != -1 && 
	mRevents.updated() && 
	(mRevents.value() & POLLIN)) {
	processInput(aExec);
    }

    // Timeout for reopening port.
    if (mTimeout.updated() && mTimeout.value() == true) {

	if (mSimulation) {
	    CArray *channels = at(XINDEX(CADDevice,channels)).arr; // 16 CADChannel element array.
	    CADChannel *chan;
	    unsigned int val;
	    mTimeout.putValue(aExec, false);


	    //
	    // Channel 0 goes 0->0x0EA60
	    //
	    chan = dynamic_cast<CADChannel *>(channels->at(0).o);
	    val = chan->adValue();
	    val = (val + 1)%0x0400;
	    chan->adValue(aExec, val);

	    //
	    // Channel 15 goes from 0x0400 to 0
	    //
	    chan = dynamic_cast<CADChannel *>(channels->at(15).o);
	    val = chan->adValue();
	    if (val> 0)
		val--;
	    else 
		val = 0x400;

	    chan->adValue(aExec, val);

// 	    DBGFMT_CLS("CADDevice::execute(): Simulation update. chan[0]=%d chan[15]=%d", 
// 		       dynamic_cast<CADChannel *>(channels->at(0).o)->adValue(),
// 		       dynamic_cast<CADChannel *>(channels->at(15).o)->adValue());

	    // Setup timeout for port increments
	    mTimeoutObj->put(aExec, XINDEX(CTimeout, duration), UFloat(0.05));
	    mTimeoutObj->put(aExec, XINDEX(CTimeout, enabled), UTrue());
	    mTimeoutObj->put(aExec, XINDEX(CTimeout, reset), UTrue());
	    return;
	}

	// Are we waiting for port reopen?
	if (mWait == WAIT_NONE)  {
	    openDevice(aExec);
	    return;
	}

	// 
	// Did we not receive a response to our init string?
	//
	if (mWait == WAIT_INIT_START)  {
	    DBGFMT_CLS("CADDevice::execute(): No init response.");
	    openDevice(aExec);
	    return;
	}
    
	DBGFMT_CLS("Got timeout outside WAIT_NONE and WAIT_INIT_START. mWait[%d]", mWait);
    }
}



// Open the (serial) device
void CADDevice::openDevice(CExecutor* aExec)
{
    TimeStamp tTimeStamp = aExec->cycleTime();
    struct termios t;
    int fl;

    if (mDescriptor != -1)
	closeDevice(aExec);

    if (mPort.value() == "")
	return;

    if (mPort.value() == "sim") {
	CArray *channels = at(XINDEX(CADDevice,channels)).arr; // 16 CADChannel element array.

	DBGFMT_CLS("CADDevice:  [%s] Will simulate chan 0 and 15", mPort.value().c_str());

	// Channel 0 goes 0->0x0E00
	dynamic_cast<CADChannel *>(channels->at(0).o)->adValue(aExec, 0);

	// Channel 15 goes 0x0E00->0
	dynamic_cast<CADChannel *>(channels->at(15).o)->adValue(aExec, 0x0400);

	mSimulation = true;
	// Setup timeout for port increments
	mTimeoutObj->put(aExec, XINDEX(CTimeout, duration), UFloat(0.05));
	mTimeoutObj->put(aExec, XINDEX(CTimeout, enabled), UTrue());
	mTimeoutObj->put(aExec, XINDEX(CTimeout, reset), UTrue());
	return;
    }

    mSimulation = false;

    DBGFMT_CLS("CADDevice: open [%s]", mPort.value().c_str());

    if ((mDescriptor = open(mPort.value().c_str(),O_RDWR|O_NOCTTY)) < 0)
	goto fail;    

    DBGFMT_CLS("CADDevice: open fd=%d", mDescriptor);
    
    if (tcgetattr(mDescriptor, &t) < 0) {
	DBGFMT_WARN("CADDevice: Could not get termio on [%s]: %s",
		    mPort.value().c_str(), strerror(errno));
    }

    cfmakeraw(&t);
    t.c_cflag = CS8 | CLOCAL | CREAD;
    t.c_iflag = IGNBRK | IGNPAR | ICRNL; // Ignore break, parity and map cr->nl
    t.c_oflag = ONLCR ;  // nl->cr
    t.c_lflag = 0; // Canonical mode. Raw mode eats more cpu.
    t.c_cc[VTIME] = 0;
    t.c_cc[VMIN] = 1;


    switch(mPortSpeed.value()) {
    case 9600:
	cfsetispeed(&t, B9600);  
	cfsetospeed(&t, B9600);  
	break;
    case 19200:
	cfsetispeed(&t, B19200);
	cfsetospeed(&t, B19200); 
	break;
    case 38400:
	cfsetispeed(&t, B38400);
	cfsetospeed(&t, B38400); 
	break;
    case 57600:
	cfsetispeed(&t, B57600);
	cfsetospeed(&t, B57600); 
	break;
    case 115200:
	cfsetispeed(&t, B115200);
	cfsetospeed(&t, B115200); 
	break;
    case 230400: 
	cfsetispeed(&t, B230400);
	cfsetospeed(&t, B230400);
	break;
    default:
	DBGFMT_WARN("CADDevice: Illegal speed using max");
    }

 
    if (tcsetattr(mDescriptor, TCSANOW, &t) < 0) {
	DBGFMT_WARN("CADDevice: Could not set termio on [%s]: %s",
		    mPort.value().c_str(), strerror(errno));
	goto fail;
    }
    
    mSource->setDescriptor(aExec, mDescriptor, POLLIN);
    tcflush(mDescriptor, TCIOFLUSH);
    initAD(aExec);

    return;
   
fail:
    DBGFMT_CLS("CADDevice: openDevice[%s] error [%s]", mPort.value().c_str(), strerror(errno));
    closeDevice(aExec);

    // Setup timeout for reopen.
    mTimeoutObj->put(aExec, XINDEX(CTimeout, duration), UFloat(1.0));
    mTimeoutObj->put(aExec, XINDEX(CTimeout, enabled), UTrue());
    mTimeoutObj->put(aExec, XINDEX(CTimeout, reset), UTrue());
}


void CADDevice::initAD(CExecutor* aExec)
{
    adWrite(mDescriptor, "\n\n\n\n\n\n");
    mState.putValue(aExec, AD_DEVICE_STATE_INIT);
    mWait = WAIT_INIT_START;

    // Setup timer for response!
    mTimeoutObj->put(aExec, XINDEX(CTimeout, duration), UFloat(0.5));
    mTimeoutObj->put(aExec, XINDEX(CTimeout, enabled), UTrue());
    mTimeoutObj->put(aExec, XINDEX(CTimeout, reset), UTrue());
}


void CADDevice::closeDevice(CExecutor* aExec)
{
    if (mDescriptor == -1)
	return;

    mWait = WAIT_NONE;
    mSource->setDescriptor(aExec, -1);
    close(mDescriptor);
    mDescriptor = -1;
    mState.putValue(aExec, AD_DEVICE_STATE_NONE);
}


void CADDevice::processInput(CExecutor* aExec)
{
    int len;
    char buf[256];

    if (readData() == -1) {
	closeDevice(aExec);
	return;
    }
	
    // Process as much as we can.
    while((len = getLine(buf, 256)) > 0) {
	if (mWait == WAIT_INIT_START) {
	    CArray *channels;
	    DBGFMT_CLS("CADDevice::processInput(): ADC card init complete.", buf[len-1]);
	    mWait = WAIT_NONE;
	    mState.putValue(aExec, AD_DEVICE_STATE_OPEN);

	    // Disable timeout
	    mTimeoutObj->put(aExec, XINDEX(CTimeout, enabled), UFalse());

	    // Setup sample channels
	    channels = at(XINDEX(CADDevice,channels)).arr;
	    for (int i=0; i < 14; ++i) 
		setSampleInterval(i, dynamic_cast<CADChannel *>(channels->at(i).o)->sampleInterval());

	    continue;
	}

	//
	// Check if we have analog data. 
	//
	if (buf[0] == 'A' && mWait == WAIT_NONE && len == 11) {
	    unsigned int chan_ind = getHex(buf + 1, 2); // Channel to update
	    unsigned int value = (getHex(buf + 3, 4) & 0xFFFF); // Actual AD value
	    unsigned int ts_delta = getHex(buf + 7, 4); // ms since last sample
	    CArray *channels = at(XINDEX(CADDevice,channels)).arr; // 16 CADChannel element array.
	    CADChannel *chan; // The actual CADChannel to update.

	    // Sanity check.
	    if (chan_ind > 15 || chan_ind < 0) {
		DBGFMT_CLS("CADDevice::processInput(): A/D Data channel out of range[%d]", chan_ind);
		continue;
	    }
	
	    DBGFMT_CLS("CADDevice::processInput(): channel[%.2d] ts_delta[%.4d] value[%d]", chan_ind, ts_delta, value);

	    // Dig out the channel object.
	    chan = dynamic_cast<CADChannel *>(channels->at(chan_ind).o);
	    if (!chan) {
		DBGFMT_WARN("CADDevice::processInput(): Could not dynamic cast channel[%d] to a CADChannel",chan_ind);
		continue;
	    }

	    // Init timestamp with cycle time if this is the first sample
	    if (chan->timeStamp() == 0)
		chan->timeStamp(aExec, aExec->cycleTime());
	    else
		chan->timeStamp(aExec, chan->timeStamp() + ts_delta);

	    // Set value
	    chan->adValue(aExec, value);
	}
    }
    return;
}
