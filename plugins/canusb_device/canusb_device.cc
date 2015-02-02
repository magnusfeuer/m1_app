//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005.
//

// 
// CAN232/CANUSB protocol reader.
//
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>


#include "canusb_device.hh"

#ifdef CAN_DEBUG
#define dbgf(f, ...) fprintf((f), __VA_ARGS__)
#else
#define dbgf(f, ...)
#endif

XOBJECT_TYPE_BOOTSTRAP(CCanUSBDevice);

#define RESPONSE_OK    13
#define RESPONSE_ERROR 7
#define RESPONSE_tOK   'z'
#define RESPONSE_TOK   'Z'

/* unsigned char => hex value */
static int x_value[256] =
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 
    8, 9, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1 };

/* hex digit => char  */
static char x_char[17] = "0123456789ABCDEF";

static void printData(FILE* f, char* mesg, char* data, int len)
{
    int i;
    fprintf(f, "%s [", mesg);
    for (i = 0; i < len; i++) {
	if (isalnum(data[i]))
	    fprintf(f, "%c", data[i]);
	else {
	    switch(data[i]) {
	    case '\n': fprintf(f, "\\n"); break;
	    case '\t': fprintf(f, "\\t"); break;
	    case '\r': fprintf(f, "\\r"); break;
	    case '\a': fprintf(f, "\\a"); break;
	    case '\b': fprintf(f, "\\b"); break;
	    default: fprintf(f, "\\%03o", data[i]); break;
	    }
	}
    }
    fprintf(f, "]\n");
}

static int canWrite(int fd, char* data, int len)
{
#ifdef CAN_DEBUG
    printData(stderr, "CanUSBDevice: send", data, len);
#endif
    return write(fd, data, len);
}

CCanUSBDevice::CCanUSBDevice(CExecutor* aExec, CBaseType *aType) :
    CCanDevice(aExec, aType),
    mWait(WAIT_NONE),
    mDescriptor(-1),
    mRevents(this),
    mSource(NULL),
    mLastAttempt(0)
{ 
    mCurrentBufferLength = 0;
    mDescriptor = -1;
    iHead = iTail = 0;
    oHead = oTail = 0;

    eventPut(aExec,XINDEX(CCanUSBDevice, revents), &mRevents);
    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);

    connect(XINDEX(CCanUSBDevice,revents), 
	    mSource, XINDEX(CFileSource,revents));

    setFlags(ExecuteEverySweep);
}

CCanUSBDevice::~CCanUSBDevice()
{
    if (mDescriptor != -1) {
	// FIXME: send close channel
	close(mDescriptor);
	mState.putValue(NULL, CAN_DEVICE_STATE_NONE);
    }
    m1Release(CFileSource, mSource);
}

void CCanUSBDevice::start(CExecutor* aExec)
{
    dbgf(stderr, "CCanUSBDevice::start()\n");
    openDevice(aExec);
    mPort.cancel(aExec);
    mPortSpeed.cancel(aExec);
    mCanSpeed.cancel(aExec);
}

void CCanUSBDevice::execute(CExecutor* aExec)
{
    if (mPort.updated() || mPortSpeed.updated() || mCanSpeed.updated()) {
	closeDevice(aExec);
	openDevice(aExec);
    }
    else if (mDescriptor != -1) {
	if (mRevents.updated()) {
	    dbgf(stderr, "Got revent = %x\n", mRevents.value());
	    if (mRevents.value() & POLLIN)
		processInput(aExec);
	}
	else if (mWait == WAIT_INIT_DRAIN) {
	    if ((aExec->cycleTime() - mLastAttempt) > 1000000) {  // 1s delay
		// No more input start setup sequence
		dbgf(stderr, "WAINT_INIT_DRAIN => SETUP\n");
		setupCAN(aExec);
	    }
	}
	else if (mWait == WAIT_RETRY) {
	    TimeStamp tTimeStamp = aExec->cycleTime();
	    if ((tTimeStamp - mLastAttempt) > 1000000) {
		mLastAttempt = tTimeStamp;
		dbgf(stderr, "WAIT_RETRY => INIT\n");
		initCAN(aExec);
	    }
	}
    }
    else if (mPort.value() != "") {
	// mDescriptor == -1 (closed) so retry after 2sec
	if ((aExec->cycleTime() - mLastAttempt) > 2000000) {
	    openDevice(aExec);
	}
    }

    if (mOutputs.updated()) {
	CCanFrame* canFrame = mOutputs.value();
	unsigned int len;
	int j;

	if ((len = canFrame->mLength.value()) > 8)
	    len = 8;

	oQueue[oHead].id = canFrame->mId.value();
	oQueue[oHead].len = len;

	for (j = 0; j < (int)len; j++)
	    oQueue[oHead].data[j] = canFrame->mData->at(j).b;
	oHead = (oHead+1) % MAX_OQUEUE_SIZE;
	if (oHead == oTail) // drop oldest frame
	    oTail = (oTail+1) % MAX_OQUEUE_SIZE;	    
    }

    // Write when something on outqueue and we are not waiting and
    // CAN bus is open.
    if ((oHead != oTail) && (mWait == WAIT_NONE) && 
	(mState.value() == CAN_DEVICE_STATE_OPEN)) {
	// FIXME: check write status before moving Tail?
	outputFrame(aExec, oQueue[oTail].id, 
		    oQueue[oTail].len, oQueue[oTail].data);
	oTail = (oTail+1) % MAX_OQUEUE_SIZE;
    }
}

// Open the (serial) device
void CCanUSBDevice::openDevice(CExecutor* aExec)
{
    TimeStamp tTimeStamp = aExec->cycleTime();
    struct termios t;
    int fl;

    mLastAttempt = tTimeStamp;

    if (mDescriptor != -1)
	closeDevice(aExec);

    if (mPort.value() == "")
	return;

    dbgf(stderr, "CCanUSBDevice: open [%s]\n", mPort.self().c_str());

#if defined(O_NDELAY) && defined(F_SETFL)
    if ((mDescriptor = open(mPort.value().c_str(),O_RDWR|O_NDELAY|O_NOCTTY)) < 0)
	goto fail;
    // Now clear O_NDELAY
    // fl = fcntl(mDescriptor, F_GETFL, 0);
    // fcntl(mDescriptor, F_SETFL, fl & ~O_NDELAY);
#else
    if ((mDescriptor = open(mPort.self().c_str(),O_RDWR|O_NOCTTY)) < 0)
	goto fail;    
#endif
    dbgf(stderr, "CCanUSBDevice: open fd=%d\n", mDescriptor);
    
    if (tcgetattr(mDescriptor, &t) < 0) {
	DBGFMT_WARN("CCanUSBDevice: Could not get termio on [%s]: %s",
		    mPort.value().c_str(), strerror(errno));
    }
    t.c_iflag = IGNBRK | IGNPAR;
    t.c_oflag = 0;
    t.c_cflag = CS8 | CLOCAL | CREAD;
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
    default:
	fprintf(stderr, "CCanUSBDevice: Illegal speed using max\n");
    case 230400: 
	cfsetispeed(&t, B230400);
	cfsetospeed(&t, B230400);
	break;
    }
    t.c_lflag = 0;
    t.c_cc[VTIME] = 0;
    t.c_cc[VMIN] = 1;
 
    if (tcsetattr(mDescriptor, TCSANOW, &t) < 0) {
	DBGFMT_WARN("CCanUSBDevice: Could not set termio on [%s]: %s",
		    mPort.value().c_str(), strerror(errno));
	goto fail;
    }
    
    mSource->setDescriptor(aExec, mDescriptor, POLLIN);

    tcflush(mDescriptor, TCIOFLUSH);

    initCAN(aExec);

    return;
   
fail:
    fprintf(stderr, "CCanUSBDevice: openDevice error [%s]\n",
	    strerror(errno));
    closeDevice(aExec);
}


void CCanUSBDevice::initCAN(CExecutor* aExec)
{
    mLastAttempt = aExec->cycleTime();
    canWrite(mDescriptor, "C\r\r\r\r", 5);
    mState.putValue(aExec, CAN_DEVICE_STATE_INIT);
    mWait = WAIT_INIT_DRAIN;
}


// Setup CAN speed
void CCanUSBDevice::setupCAN(CExecutor* aExec)
{
    switch(mCanSpeed.value()) {
    case 10:   canWrite(mDescriptor, "S0\r", 3); break;
    case 20:   canWrite(mDescriptor, "S1\r", 3); break;
    case 50:   canWrite(mDescriptor, "S2\r", 3); break;
    case 100:  canWrite(mDescriptor, "S3\r", 3); break;
    case 125:  canWrite(mDescriptor, "S4\r", 3); break;
    case 250:  canWrite(mDescriptor, "S5\r", 3); break;
    case 500:  canWrite(mDescriptor, "S6\r", 3); break;
    case 800:  canWrite(mDescriptor, "S7\r", 3); break;
    case 1000: canWrite(mDescriptor, "S8\r", 3); break;
    default:
	// FIXME: calculte BTR0/BTR1 and use sxxyy command
	fprintf(stderr, "CCanUSBDevice: Illegal can speed [%d]\n",
		mCanSpeed.value());
	closeDevice(aExec);
	return;
    }
    mWait  = WAIT_INIT_SETUP;
    mState.putValue(aExec, CAN_DEVICE_STATE_INIT);
}



void CCanUSBDevice::closeDevice(CExecutor* aExec)
{
    if (mDescriptor == -1)
	return;
    if (mState.value() == CAN_DEVICE_STATE_OPEN)
	closeCAN(aExec);
    mWait = WAIT_NONE;
    mSource->setDescriptor(aExec, -1);
    close(mDescriptor);
    mDescriptor = -1;
    mState.putValue(aExec, CAN_DEVICE_STATE_NONE);
}


// Open the CAN channel "O<CR>"
void CCanUSBDevice::openCAN(CExecutor* aExec)
{
    if ((mDescriptor == -1) || (mState.value() == CAN_DEVICE_STATE_OPEN))
	return;
    canWrite(mDescriptor, "O\r", 2);
    mWait = WAIT_INIT_OPEN;
}

// Close the CAN channel "C<CR>"
void CCanUSBDevice::closeCAN(CExecutor* aExec)
{
    if ((mDescriptor == -1) || (mState.value() != CAN_DEVICE_STATE_OPEN))
	return;
    canWrite(mDescriptor, "C\r", 2);
    mWait = WAIT_CLOSE;
}

void CCanUSBDevice::processInput(CExecutor* aExec)
{
    int save_pos;
    int frame_format;
    int frame_id_len;
    int i;
    int n;

    if (mCurrentBufferLength < sizeof(mReadBuffer)) {
	n = read(mDescriptor, mReadBuffer+mCurrentBufferLength,
		 sizeof(mReadBuffer)-mCurrentBufferLength);
	if (n >= 0) {
#ifdef CAN_DEBUG
	    printData(stderr, "CanUSBDevice: read", 
		      mReadBuffer+mCurrentBufferLength, n);
#endif
	    mCurrentBufferLength += n;
	}
	else {
	    if (errno == ENXIO) {
		mCurrentBufferLength = 0;
		// device removed ?
		closeDevice(aExec);
		return;
	    }
	    if (errno == EAGAIN) {
		// well, we need to try again
		return;
	    }
	    fprintf(stderr, "CCanUSBDevice: read error %d[%s]\n",
		    errno, strerror(errno));
	    return;
	}
    }

    if (mWait == WAIT_INIT_DRAIN) {
	mCurrentBufferLength = 0;
	return;
    }

    i = 0;
    n = mCurrentBufferLength;

again:
    while(i < n) {
	save_pos = i;   // save possition on underflow
	switch(mReadBuffer[i++]) {
	case RESPONSE_OK:
	    switch(mWait) {
	    case WAIT_INIT_SETUP:
		mWait = WAIT_NONE;
		dbgf(stderr, "CCanUSBDevice: WAIT_INIT_SETUP OK\n");
		openCAN(aExec);
		break;
	    case WAIT_INIT_OPEN:
		mWait = WAIT_NONE;
		mState.putValue(aExec, CAN_DEVICE_STATE_OPEN);
		dbgf(stderr, "CCanUSBDevice: WAIT_INIT_OPEN OK\n");
		break;
	    case WAIT_CLOSE:
		mWait = WAIT_NONE;
		mState.putValue(aExec, CAN_DEVICE_STATE_CLOSED);
		dbgf(stderr, "CCanUSBDevice: WAIT_CLOSE OK\n");
		break;
	    default:
		dbgf(stderr, "CCanUSBDevice: WAIT ERROR\n");
		break;
	    }
	    break;
		
	case RESPONSE_ERROR:
	    switch(mWait) {
	    case WAIT_NONE:
		dbgf(stderr, "CCanUSBDevice: WAIT_NONE ERROR???\n");
		break;
	    case WAIT_INIT_OPEN:
		mWait = WAIT_NONE;
		dbgf(stderr, "CCanUSBDevice: WAIT_INIT_OPEN ERROR(RETRY)\n");
		mWait = WAIT_RETRY;
		break;
	    case WAIT_INIT_SETUP:
		dbgf(stderr, "CCanUSBDevice: WAIT_SETUP ERROR (RETRY)\n");
		mWait = WAIT_RETRY;
		break;
	    case WAIT_CLOSE:
		mWait = WAIT_NONE;
		dbgf(stderr, "CCanUSBDevice: WAIT_CLOSE ERROR\n");
		mState.putValue(aExec, CAN_DEVICE_STATE_CLOSED);
		break;
	    case WAIT_t:
		mWait = WAIT_NONE;
		dbgf(stderr, "CCanUSBDevice: WAIT_t ERROR\n");
		break;
	    case WAIT_T:
		mWait = WAIT_NONE;
		dbgf(stderr, "CCanUSBDevice: WAIT_T ERROR\n");
		break;
	    default:
		break;
	    }
	    break;

	case 'z':
	    switch(mWait) {
	    case WAIT_t:
		if (i >= n)
		    goto more_data;
		mWait = WAIT_NONE;
		dbgf(stderr, "CCanUSBDevice: WAIT_t OK\n");
		// just skip the cr if present
		if (mReadBuffer[i] == '\r')
		    i++;
		break;
	    default:
		dbgf(stderr, "CCanUSBDevice: WAIT ERROR\n");
		break;
	    }
	    break;

	case 'Z':
	    switch(mWait) {
	    case WAIT_T:
		if (i >= n)
		    goto more_data;
		mWait = WAIT_NONE;
		dbgf(stderr, "CCanUSBDevice: WAIT_T OK\n");
		if (mReadBuffer[i] == '\r')
		    i++;
		break;
	    default:
		dbgf(stderr, "CCanUSBDevice: WAIT ERROR\n");
		break;
	    }
	    break;

	case 't':
	    frame_format = CAN_BASE_FRAME_FORMAT;
	    frame_id_len = 3;
	    goto parse_frame;

	case 'T':
	    frame_format = CAN_EXTENDED_FRAME_FORMAT;
	    frame_id_len = 8;
	    goto parse_frame;

	default:
	    dbgf(stderr, "CCanUSBDevice: skip data %c [%d]\n",
		 mReadBuffer[i-1], mReadBuffer[i-1]);
	    break;
	}
    }

    // At this point either we process all frames replies or
    // we want to flush input
flush:
    mCurrentBufferLength = 0;
    return;

more_data:
    // Save data from save_pos to mCurrentBufferLength
    memmove(mReadBuffer, mReadBuffer+save_pos, mCurrentBufferLength-save_pos);
    mCurrentBufferLength -= save_pos;
    return;

parse_frame: 
{
    unsigned char data[8];  // data frame
    unsigned long id = 0;
    int len = 0;
    int j;

    // process the id = iii | iiiiiiii
    id = 0;
    for (j = 0; j < frame_id_len; j++) {
	int x;

	if (i>=n) goto more_data;
	if ((x = x_value[mReadBuffer[i]]) < 0) goto error_frame;
	id = (id << 4) | x;
	i++;
    }

    if (i>=n) goto more_data;
    if ((len = x_value[mReadBuffer[i]]) < 0) goto error_frame;
    if (len > 8) goto error_length;
    i++;

    for (j = 0; j < len; j++) {
	unsigned char d = 0;
	int x;
	if (i>=n) goto more_data;
	if ((x = x_value[mReadBuffer[i]]) < 0) goto error_frame;
	d = x;
	i++;
	if (i>=n) goto more_data;
	if ((x = x_value[mReadBuffer[i]]) < 0) goto error_frame;
	data[j] = (d << 4) | x;
	i++;
    }
    if (i>=n) goto more_data;
    if (mReadBuffer[i] == RESPONSE_OK) {
	i++;
	inputFrame(aExec, id, len, data);
	goto again;
    }
}

error_frame:
  fprintf(stderr, "CCanUSBDevice: bad frame data: \n");
  goto error;

error_length:
  fprintf(stderr, "CCanUSBDevice: bad frame length: \n");
  goto error;

error:
{
#ifdef CAN_DEBUG
    int j;
    for (j = save_pos; j < mCurrentBufferLength; j++)
	fprintf(stderr, "%c", mReadBuffer[j]);
    fprintf(stderr, "\n");
#endif
    i = save_pos + 1;  // retry from next character
    goto again;
}


}


void CCanUSBDevice::inputFrame(CExecutor* aExec,unsigned long id, int len,
			       unsigned char* data)
{
    int i;
    CArray* iArray;

    dbgf(stderr, "RECEIVED FRAME:\n");
    dbgf(stderr, "ID=%8X, LEN=%d, DATA=", id, len);
    for (i = 0; i < len; i++)
	dbgf(stderr, "%02X ", data[i]);
    dbgf(stderr, "\n");

    // Scan trough input list and match frames to update
    iArray = at(XINDEX(CCanDevice,inputs)).arr;
    for (i = 0; i < iArray->size(); i++) {
	CCanFrame* canFrame = (CCanFrame*) iArray->at(i).o;
	//
	// FIXME: if we have multiple match on a frame, then we should
	//  probably queue the next matching and use it later 
	//
	if (canFrame->match(id)) {
	    int j;
	    canFrame->mId.putValue(aExec,  id);
	    canFrame->mLength.putValue(aExec, len);
	    for (j = 0; j < len; j++)
		canFrame->mData->put(aExec, j, UByte(data[j]));
	    for (j = len; j < 8; j++)
		canFrame->mData->put(aExec, j, UByte(0));
	    break;
	}
    }
}


void CCanUSBDevice::outputFrame(CExecutor* aExec,unsigned long id, int len,
				unsigned char* data)
{
    char command[1+8+1+16+1+1];  // buffer to cover extended format
    int i, j;

    if (mFormat.value() == CAN_EXTENDED_FRAME_FORMAT) {
	if ((len < 0) || (len > 8)) goto length_error;
	if (id > 0x1FFFFFFF) goto id_error;
	command[0] = 'T';
	command[1] = x_char[(id >> 28) & 0xf];
	command[2] = x_char[(id >> 24) & 0xf];
	command[3] = x_char[(id >> 20) & 0xf];
	command[4] = x_char[(id >> 16) & 0xf];
	command[5] = x_char[(id >> 12) & 0xf];
	command[6] = x_char[(id >> 8) & 0xf];
	command[7] = x_char[(id >> 4) & 0xf];
	command[8] = x_char[id & 0xf];
	command[9] = x_char[len & 0xf];
	j = 10;
	for (i = 0; i < len; i++) {
	    command[j]   = x_char[data[i]>>4];
	    command[j+1] = x_char[data[i]&0xf];
	    j += 2;
	}
	command[j] = '\r';
	j++;
	command[j] = '\0';
	mWait = WAIT_T;
    }
    else {
	// CAN_BASE_FRAME_FORMAT
	if ((len < 0) || (len > 8)) goto length_error;
	if (id > 0x7FF) goto id_error;
	command[0] = 't';
	command[1] = x_char[(id >> 8) & 0xf];
	command[2] = x_char[(id >> 4) & 0xf];
	command[3] = x_char[id & 0xf];
	command[4] = x_char[len & 0xf];
	j = 5;
	for (i = 0;  i < len; i++) {
	    command[j]   = x_char[data[i]>>4];
	    command[j+1] = x_char[data[i]&0xf];
	    j += 2;
	}
	command[j] = '\r';
	j++;
	command[j] = '\0';
	mWait = WAIT_t;
    }
    if (canWrite(mDescriptor, command, j) != j) {
	fprintf(stderr, "CCanUSBDevice: write error [%s]\n", strerror(errno));
	mWait = WAIT_NONE;
    }
    return;

length_error:
    fprintf(stderr, "CCanUSBDevice: frame length error\n");
    return;
id_error:
    fprintf(stderr, "CCanUSBDevice: frame id error\n");
    return;
}
