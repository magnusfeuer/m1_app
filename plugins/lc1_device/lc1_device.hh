//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
// Innovate LC1 protocol
//
#ifndef __LC1_DEVICE_HH__
#define __LC1_DEVICE_HH__

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
} CLC1State;


ENUM_TYPE(CLC1State, "LC1State",
	  ENUMERATION(no_connection,      NO_CONNECTION),
	  ENUMERATION(running,            RUNNING)
    );


#define LC1_INPUT_BUFFER_SIZE 10240

class CLC1Device: public CExecutable {
public:
    XOBJECT_TYPE(CLC1Device, "LC1Device", "Zeitronix Zt-2 driver",
		 (CLC1Device_port,
		  CLC1Device_state,
		  CLC1Device_revents,
		  CLC1Device_read_timeout,

		  CLC1Device_lam,
		  CLC1Device_lamUsage
		  ),

		 XFIELD(CLC1Device, Q_PUBLIC, port, event_string_type(), ""),
		 XFIELD(CLC1Device, Q_PUBLIC, state, CEventType::create(CLC1StateType::singleton(), E_INOUT), ""),
		 XFIELD(CLC1Device, Q_PUBLIC, revents, event_signed_type(), ""),
		 XFIELD(CLC1Device, Q_PUBLIC, read_timeout, event_bool_type(), ""),
		 XFIELD(CLC1Device, Q_PUBLIC, lam, event_float_type(), "")
		 );
public:
    CLC1Device(CExecutor* aExec,
		CBaseType *aType = CLC1DeviceType::singleton());
    ~CLC1Device(void);
    void execute(CExecutor* aExec);

private:
    void openPort(CExecutor *aExec, TimeStamp aTimesTamp);

private:
    bool getData(void); // Do a read into raw buffer. Do not process
    int extractData(uchar *aBuffer, int aLength); // Read data from raw buffer only.
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


    //
    // File descriptor object that ties
    // mDescriptor into the CSweeper
    // poll/scheduler system
    //
    CFileSource *mSource;

    //
    // Read timeout timer
    //
    CTimeout *mReadTimeoutTimer;
    EventBool mReadTimeout;

    // Input buffer
    uchar mInputBuffer[LC1_INPUT_BUFFER_SIZE];
    int mInputBufferLen;
    bool mFirstRead; // Is this first read after device has been opened?

    // Channels
    EventFloat mLam;
};

#endif // __LC1_DEVICE_HH__

