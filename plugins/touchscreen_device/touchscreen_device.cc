// 
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005.
//


//
// Touch screen driver for /dev/input/event
//
#include "touchscreen_device.hh"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>

XOBJECT_TYPE_BOOTSTRAP(CTouchScreenDevice);


CTouchScreenDevice::CTouchScreenDevice(CExecutor* aExec,CBaseType *aType):
    CInputDeviceBase(aExec, aType),
    mDevice(this),
    mLastOpenAttempt(0),
    mButtonDown(0)
{ 
    mDevice.putValue(aExec, "");
    for (int i = 0; i < MAX_EVENT_DEVICE; ++i)  {
	mRevents[i] = new EventSigned(this);
	mFileSource[i] = new CFileSource(aExec);
	m1Retain(CFileSource, mFileSource[i]);
	mDescriptor[i] = -1;
    }

    eventPut(aExec, XINDEX(CTouchScreenDevice, device), &mDevice);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents0), mRevents[0]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents1), mRevents[1]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents2), mRevents[2]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents3), mRevents[3]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents4), mRevents[4]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents5), mRevents[5]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents6), mRevents[6]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents7), mRevents[7]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents8), mRevents[8]);
    eventPut(aExec, XINDEX(CTouchScreenDevice, revents9), mRevents[9]);

    setFlags(ExecuteEverySweep);

}

CTouchScreenDevice::~CTouchScreenDevice(void) 
{


    for (int i = 0; i < MAX_EVENT_DEVICE; ++i) {
	shutdownDevice(0, i);
	m1Release(CFileSource, mFileSource[i]);
	delete mRevents[i];

	if (mDescriptor[i] != -1)
	    close(mDescriptor[i]);
    }
}


void CTouchScreenDevice::unknownEvent(struct input_event *a)
{
    DBGFMT("CTouchScreenDevice(): Unknown event Type[%d/%X] Code[%d/%X], Val[%d/%X]", 
 	   a->type,
 	   a->type,
 	   a->code,
 	   a->code,
 	   a->value,
	   a->value);
}

void CTouchScreenDevice::setupDevice(CExecutor* aExec, int aIndex) 
{
    if (mDescriptor[aIndex] == -1)
	return;

    // Register device with file descriptor
    mFileSource[aIndex]->setDescriptor(aExec, mDescriptor[aIndex]);

    switch (aIndex) {
    case 0:
	connect(XINDEX(CTouchScreenDevice, revents0), mFileSource[0], XINDEX(CFileSource,revents));
	break;
    case 1:
	connect(XINDEX(CTouchScreenDevice, revents1), mFileSource[1], XINDEX(CFileSource,revents));
	break;
    case 2:
	connect(XINDEX(CTouchScreenDevice, revents2), mFileSource[2], XINDEX(CFileSource,revents));
	break;
    case 3:
	connect(XINDEX(CTouchScreenDevice, revents3), mFileSource[3], XINDEX(CFileSource,revents));
	break;
    case 4:
	connect(XINDEX(CTouchScreenDevice, revents4), mFileSource[4], XINDEX(CFileSource,revents));
	break;
    case 5:
	connect(XINDEX(CTouchScreenDevice, revents5), mFileSource[5], XINDEX(CFileSource,revents));
	break;
    case 6:
	connect(XINDEX(CTouchScreenDevice, revents6), mFileSource[6], XINDEX(CFileSource,revents));
	break;
    case 7:
	connect(XINDEX(CTouchScreenDevice, revents7), mFileSource[7], XINDEX(CFileSource,revents));
	break;
    case 8:
	connect(XINDEX(CTouchScreenDevice, revents8), mFileSource[8], XINDEX(CFileSource,revents));
	break;
    case 9:
	connect(XINDEX(CTouchScreenDevice, revents9), mFileSource[9], XINDEX(CFileSource,revents));
	break;
    }

    return;
}


void CTouchScreenDevice::shutdownDeviceByDescriptor(CExecutor* aExec, int aDescriptor) 
{
    if (aDescriptor == -1)
	return;

    for(int i = 0; i < MAX_EVENT_DEVICE; ++i)
	if (mDescriptor[i] == aDescriptor) {
	    shutdownDevice(aExec, i);
	    return;
	}

    return;
}
void CTouchScreenDevice::shutdownDevice(CExecutor* aExec, int aIndex) 
{
    if (mDescriptor[aIndex] == -1)
	return;

    switch (aIndex) {
    case 0:
	disconnect(XINDEX(CTouchScreenDevice,revents0));
	break;
    case 1:
	disconnect(XINDEX(CTouchScreenDevice,revents1));
	break;
    case 2:
	disconnect(XINDEX(CTouchScreenDevice,revents2));
	break;
    case 3:
	disconnect(XINDEX(CTouchScreenDevice,revents3));
	break;
    case 4:
	disconnect(XINDEX(CTouchScreenDevice,revents4));
	break;
    case 5:
	disconnect(XINDEX(CTouchScreenDevice,revents5));
	break;
    case 6:
	disconnect(XINDEX(CTouchScreenDevice,revents6));
	break;
    case 7:
	disconnect(XINDEX(CTouchScreenDevice,revents7));
	break;
    case 8:
	disconnect(XINDEX(CTouchScreenDevice,revents7));
	break;
    case 9:
	disconnect(XINDEX(CTouchScreenDevice,revents9));
	break;
    }

    close(mDescriptor[aIndex]);
    mDescriptor[aIndex] = -1;
    if (aExec)
	mFileSource[aIndex]->setDescriptor(aExec, -1);

    return;
}

void CTouchScreenDevice::execute(CExecutor* aExec)
{
    Time tTime = m1_TimeStampToTime(aExec->timeStamp());
    struct input_event *evt;
    unsigned char buf[sizeof(struct input_event) * 250];
    int read_len = 0;
    int parsed_len = 0;
    int i;
    int fd_cnt;
    struct pollfd fd[MAX_EVENT_DEVICE];

    //
    // Check for provisioning data.
    //
    if (mDevice.updated()) {
	
	for (i = 0; i < MAX_EVENT_DEVICE; ++i) 
	    shutdownDevice(aExec, i);

    }
	
	    // Close all old devices.
	
    if (mDevice.updated() || ((tTime - mLastOpenAttempt) > 2000)) {
	// Does device contain a formatting string?
	
	if (mDevice.value().find('%') != string::npos) {
	    errno = 0;
	    //
	    // Open all devices until we run into a file not found situation.
	    //
	    for (i = 0; i < MAX_EVENT_DEVICE; ++i) {
		// Skip if already open and happy.

		if (mDescriptor[i] != -1) 
		    continue;
		char path[256];

		sprintf(path, mDevice.value().c_str(), i);

		if ((mDescriptor[i] = open(path, O_RDONLY)) != -1) {
		    DBGFMT("CTouchScreenDevice::execute(): Opened device [%s]\n", path);
		    // Register device with file descriptor
		    setupDevice(aExec,i);
		}
		else
		    DBGFMT("CTouchScreenDevice:SetMember(): Could not open [%s]: %s\n", path, strerror(errno));

	    }

	} else { 
	    //
	    // No % in mDevice. Open with unprocessed name.
	    //
	    if ((mDescriptor[0] = open(mDevice.value().c_str(), O_RDONLY)) != -1)  {
		// Register device with file descriptor
		setupDevice(aExec, 0);
		DBGFMT("CTouchScreenDevice::execute(): Opened device [%s]", mDevice.value().c_str());
	    } else 
		DBGFMT("CTouchScreenDevice:SetMember(): Could not open [%s]: %s",
		       mDevice.value().c_str(), strerror(errno));
	}
	mLastOpenAttempt = tTime;
	return;
    }

    //
    // We will only be called if mDescriptor is ready since we only subscribe
    // to CFileSource's revents, which is tied to mDescriptor
    //
    i = MAX_EVENT_DEVICE;
    while(i--) {
	if (mDescriptor[i] != -1 && mRevents[i]->updated())
	    break;
    }
    if (i == -1)
	return;


    //
    // Sometimes the driver triggers us, but we do not get any data.
    //
    fd_cnt = 0;
    for (i=0; i < MAX_EVENT_DEVICE; ++i) {
	if (mDescriptor[i] == -1)
	    continue;

	fd[fd_cnt].events = POLLIN | POLLHUP;
	fd[fd_cnt].revents = 0;
	fd[fd_cnt].fd = mDescriptor[i];
	++fd_cnt;
    }

    // Bug workaround. Sometimes we get invoked without traffic.
    // We need to check that each descriptor we attemt to read is
    // ready
    poll(fd, fd_cnt, 0);
    
    //
    // Go through all open descriptors and read data
    //
    for (i=0; i < MAX_EVENT_DEVICE; ++i) {
	int j;

	// Skip if descriptor is not open.
	if (mDescriptor[i] == -1)
	    continue;
	
	//
	// Go through the poll set and locate our descriptor, and validate that it
	// is ready.
	//
	j = fd_cnt;
	while(j--)  {
	    if (fd[j].revents & POLLHUP) {
		shutdownDeviceByDescriptor(aExec, fd[j].fd);
		continue;
	    }
		    
	    if (fd[j].fd == mDescriptor[i] && (fd[j].revents & POLLIN))
		break;
	    
	}	
	
	// mDescriptor[i] is not ready for reading
	if (j == -1)
	    continue;
	
	if ((read_len = read(mDescriptor[i], buf, sizeof(struct input_event) * 100)) <= 0) {
	    DBGFMT("CTouchScreenDevice::execute(): Could not read descriptor. Read length[%d].", read_len);
	    shutdownDevice(aExec, i);
	    continue;
	}
	parsed_len = 0;

	//
	// Iterate through all data read up until the last complete package.
	//
	evt = (struct input_event *) buf;

	while(read_len - parsed_len >= sizeof(struct input_event)) {
	    switch(evt->type) {
	    case EV_SYN: 	    // Zero sync
		break;

	    case EV_KEY:            // Code BTN_TOUCH - A touch down or up, or key down/up
		// Keyboard event?
		if (evt->code <= KEY_UNKNOWN) {
		    if (evt->value != 0) {
			keyDown(aExec, evt->code, tTime - 1 );
		    }
		    else {
			keyUp(aExec, evt->code);
		    }
		    break;
		}

		if (evt->code != BTN_TOUCH && evt->code != BTN_LEFT) {
		    unknownEvent(evt);
		    break;
		}

		if (evt->value != 0) {
		    mButtonDown = tTime - 1;
		    buttonDown(aExec, 1, 1);
		}
		else {
		    buttonUp(aExec, 1);
		    mButtonDown = 0;
		}
		break;

	    case EV_ABS: 		// X or Y position
		if (evt->code == ABS_X || evt->code == ABS_RX) { // ABS_RX = Xenarc DVI weirdness
		    rawAbsoluteX(aExec, evt->value);
		    break;
		}

		if (evt->code == ABS_Y || evt->code == ABS_Z ) { // ABS_Z = Xenarc DVI weirdness.
		    rawAbsoluteY(aExec, evt->value);
		    break;
		}
		break;

	    case EV_REL: 		// X or Y position
		if (evt->code == REL_X) {
		    rawRelativeX(aExec, evt->value);
		    break;
		}

		if (evt->code == REL_Y) {
		    rawRelativeY(aExec, evt->value);
		    break;
		}
		break;

		

	    case EV_MSC:
		break;

	    default:
	        unknownEvent(evt);
		break;
	    }
	    evt++;
	    parsed_len += sizeof(struct input_event);
	}
    }
    return;

}
