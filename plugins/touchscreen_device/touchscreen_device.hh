//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005.
//

//
// Fulitsuy touchscreen driver
//
#ifndef __TOUCHSCREEN_DEVICE_H__
#define __TOUCHSCREEN_DEVICE_H__

#include "input_device.hh"
#define MAX_EVENT_DEVICE 10
class CTouchScreenDevice: public CInputDeviceBase {
public:
    XDERIVED_OBJECT_TYPE(CTouchScreenDevice, CInputDeviceBase,
			"TouchScreenDevice", 
			 "Touch device using the standard input event system for linux.",
			 (CTouchScreenDevice_device,
			  CTouchScreenDevice_revents0,
			  CTouchScreenDevice_revents1,
			  CTouchScreenDevice_revents2,
			  CTouchScreenDevice_revents3,
			  CTouchScreenDevice_revents4,
			  CTouchScreenDevice_revents5,
			  CTouchScreenDevice_revents6,
			  CTouchScreenDevice_revents7,
			  CTouchScreenDevice_revents8,
			  CTouchScreenDevice_revents9,
			  CTouchScreenDevice_revents10),
			 XFIELD(CTouchScreenDevice, Q_PUBLIC, device, 
				event_string_type(), "Device (/dev/input/event%d) Provide a printf formatter to open multiple devices"),

			 // A bit unsure on the trigger sequence for individual array members. So we'll do this for now. FIXME.
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents0, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents1, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents2, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents3, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents4, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents5, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents6, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents7, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents8, input_signed_type(), "Input ready"),
			 XFIELD(CTouchScreenDevice, Q_PRIVATE,revents9, input_signed_type(), "Input ready")
			 );

public:
    CTouchScreenDevice(CExecutor* aExec, 
		       CBaseType *aType = CTouchScreenDeviceType::singleton());
    ~CTouchScreenDevice(void);
    void execute(CExecutor* aExec);

private:
    void setupDevice(CExecutor* aExec,int aIndex);
    void shutdownDevice(CExecutor* aExec, int aIndex);
    void shutdownDeviceByDescriptor(CExecutor* aExec, int aDescriptor);
    void unknownEvent(struct input_event *aEvent);    

    EventSigned *mRevents[MAX_EVENT_DEVICE];
    EventString mDevice;

    int mDescriptor[MAX_EVENT_DEVICE];                   // File descriptor for reading touchscreen data.
    CFileSource* mFileSource[MAX_EVENT_DEVICE];  // Ties into file sweeper poll system
    Time mLastOpenAttempt;
    TimeStamp mButtonDown; // When screen was originally pressed.
};

#endif 
