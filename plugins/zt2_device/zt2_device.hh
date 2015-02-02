//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
// Zeitronix ZT2 device.
//
#ifndef __ZT2_DEVICE_HH__
#define __ZT2_DEVICE_HH__

#include <stdio.h>
#include "component.hh"
#include "time_sensor.hh"

typedef unsigned char uchar;

//
// Run state 
//
typedef enum { 
    NO_CONNECTION = 1, // No device found
    RUNNING = 2, // Running.
} CZT2State;


ENUM_TYPE(CZT2State, "ZT2State",
	  ENUMERATION(no_connection,      NO_CONNECTION),
	  ENUMERATION(running,            RUNNING)
    );


#define ZT2_INPUT_BUFFER_SIZE 10240

class CZT2Device: public CExecutable {
public:
    XOBJECT_TYPE(CZT2Device, "ZT2Device", "Zeitronix Zt-2 driver",
		 (CZT2Device_port,
		  CZT2Device_state,
		  CZT2Device_revents,
		  CZT2Device_read_timeout,
		  CZT2Device_cylinder_count, // Number of cylinders in engine. Needed for RPM calc
		  CZT2Device_rpm_multiplier, // Multiplier applied to raw rpm value.

		  CZT2Device_lam,
		  CZT2Device_egt,
		  CZT2Device_rpm,
		  CZT2Device_map,
		  CZT2Device_tps,
		  CZT2Device_user1
		  ),

		 XFIELD(CZT2Device, Q_PUBLIC, port, event_string_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, state, CEventType::create(CZT2StateType::singleton(), E_INOUT), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, revents, event_signed_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, read_timeout, event_bool_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, cylinder_count, event_signed_type(), "Nr of cylinders in engine."),
		 XFIELD(CZT2Device, Q_PUBLIC, rpm_multiplier, event_float_type(), "Multiplier to apply to rpm."),
		 
		 XFIELD(CZT2Device, Q_PUBLIC, lam, event_float_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, egt, event_float_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, rpm, event_float_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, map, event_float_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, tps, event_float_type(), ""),
		 XFIELD(CZT2Device, Q_PUBLIC, user1, event_float_type(), "")
		 );
public:
    CZT2Device(CExecutor* aExec,
		CBaseType *aType = CZT2DeviceType::singleton());
    ~CZT2Device(void);
    void start(CExecutor* aExec);
    void execute(CExecutor* aExec);

private:
    void openPort(CExecutor *aExec);

private:
 
    bool getData(void); // Do a read into raw buffer. Do not process
    bool extractData(uchar *aBuffer, int aLength); // Read data from raw buffer only.
    int availableData(void) { return mInputBufferLen; }
    bool stopStreaming(void);
    bool setupStreamingProfile(void);
    bool processStream(CExecutor* aExec);
    
    // Serial file descriptor
    int mDescriptor;

    //
    // Port or file we are to read data from
    //
    EventString mPort;

    //
    // The poll revents variable subscribing to the mFileSource
    //
    EventSigned mRevents;
    EventSigned mState;

    // Number of cylinders.
    EventSigned mCylinderCount;

    // RPM multiplier
    EventFloat mRPMMultiplier;

    //
    // File descriptor object that ties
    // mDescriptor into the CSweeper
    // poll/scheduler system
    //
    CFileSource *mSource;

    // Input buffer
    uchar mInputBuffer[ZT2_INPUT_BUFFER_SIZE];
    int mInputBufferLen;
    bool mFirstRead; // Is this first read after device has been opened?

    // Channels
    EventFloat mLam;
    EventFloat mEgt;
    EventFloat mRpm;
    EventFloat mMap;
    EventFloat mTps;
    EventFloat mUser1;
};

#endif // __ZT2_DEVICE_HH__

