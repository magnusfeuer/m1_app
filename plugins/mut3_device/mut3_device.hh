//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
// Mut3 1 protocol, done for Nissan Skyline GT(S) 2001 for Fast and the Furious 4
//
#ifndef __MUT3_DEVICE_HH__
#define __MUT3_DEVICE_HH__

#include <stdio.h>
#include "component.hh"
#include "time_sensor.hh"

typedef unsigned char uchar;

//
// Run state 
//
typedef enum { 
    UNDEFINED = 0, // No device found
    CONNECTING = 1, // Device com being setup.
    RUNNING = 3, // Running.
} CMUT3State;

#define MUT3_PID_COUNT 14
#define MUT3_INPUT_BUFFER_SIZE 10240

#define MUT3_BAUDRATE 15625 // Strange baud rate.

ENUM_TYPE(CMUT3State, "MUT3State",
	  ENUMERATION(undefined,          UNDEFINED),
	  ENUMERATION(connecting,         CONNECTING),
	  ENUMERATION(running,            RUNNING)
	  );


class CMUT3Device: public CExecutable {
public:
    XOBJECT_TYPE(CMUT3Device, "MUT3Device", "MUT III driver",
		 (CMUT3Device_port,
		  CMUT3Device_protocol,
		  CMUT3Device_state,
		  CMUT3Device_iso_gap_time,
		  CMUT3Device_revents,
		  CMUT3Device_port_reopen_timeout,
		  CMUT3Device_read_timeout,
		  CMUT3Device_port_setup_timeout,
		  CMUT3Device_latency,

		  CMUT3Device_timing, 
		  CMUT3Device_rpm, 
		  CMUT3Device_spd, 
		  CMUT3Device_lam_1,
		  CMUT3Device_lam_2,
		  CMUT3Device_maf, 
		  CMUT3Device_tps, 
		  CMUT3Device_ect, 
		  CMUT3Device_map, 
		  CMUT3Device_act, 
		  CMUT3Device_bap, 
		  CMUT3Device_vbat, 
		  CMUT3Device_fpw, 
		  CMUT3Device_wgdc, // Wastegate duty cycle.

		  CMUT3Device_timingUsage,
		  CMUT3Device_rpmUsage,
		  CMUT3Device_spdUsage,
		  CMUT3Device_lam_1Usage,
		  CMUT3Device_lam_2Usage,
		  CMUT3Device_mafUsage,
		  CMUT3Device_tpsUsage,
		  CMUT3Device_ectUsage,
		  CMUT3Device_mapUsage,
		  CMUT3Device_actUsage,
		  CMUT3Device_bapUsage,
		  CMUT3Device_vbatUsage,
		  CMUT3Device_fpwUsage,
		  CMUT3Device_wgdcUsage,

		  CMUT3Device_timingSupported,
		  CMUT3Device_rpmSupported,
		  CMUT3Device_spdSupported,
		  CMUT3Device_lam_1Supported,
		  CMUT3Device_lam_2Supported,
		  CMUT3Device_mafSupported,
		  CMUT3Device_tpsSupported,
		  CMUT3Device_ectSupported,
		  CMUT3Device_mapSupported,
		  CMUT3Device_actSupported,
		  CMUT3Device_bapSupported,
		  CMUT3Device_vbatSupported,
		  CMUT3Device_fpwSupported,
		  CMUT3Device_wgdcSupported,

		  CMUT3Device_timingMin,
		  CMUT3Device_rpmMin,
		  CMUT3Device_spdMin,
		  CMUT3Device_lam_1Min,
		  CMUT3Device_lam_2Min,
		  CMUT3Device_mafMin,
		  CMUT3Device_tpsMin,
		  CMUT3Device_ectMin,
		  CMUT3Device_mapMin,
		  CMUT3Device_actMin,
		  CMUT3Device_bapMin,
		  CMUT3Device_vbatMin,
		  CMUT3Device_fpwMin,
		  CMUT3Device_wgdcMin,

		  CMUT3Device_timingMax,
		  CMUT3Device_rpmMax,
		  CMUT3Device_spdMax,
		  CMUT3Device_lam_1Max,
		  CMUT3Device_lam_2Max,
		  CMUT3Device_mafMax,
		  CMUT3Device_tpsMax,
		  CMUT3Device_ectMax,
		  CMUT3Device_mapMax,
		  CMUT3Device_actMax,
		  CMUT3Device_bapMax,
		  CMUT3Device_vbatMax,
		  CMUT3Device_fpwMax,
		  CMUT3Device_wgdcMax
		  ),


		 XFIELD(CMUT3Device, Q_PUBLIC, port, event_string_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, state, CEventType::create(CMUT3StateType::singleton(), E_INOUT), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, revents, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, port_reopen_timeout, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, read_timeout, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, port_setup_timeout, event_bool_type(), ""),

		 XFIELD(CMUT3Device, Q_PUBLIC, timing, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, rpm, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, spd, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_1, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_2, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, maf, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, tps, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, ect, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, map, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, act, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, bap, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, vbat, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, fpw, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, wgdc, event_float_type(), ""),

		 XFIELD(CMUT3Device, Q_PUBLIC, timingUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, rpmUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, spdUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_1Usage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_2Usage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mafUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, tpsUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, ectUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mapUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, actUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, bapUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, vbatUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, fpwUsage, event_signed_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, wgdcUsage, event_float_type(), ""),

		 XFIELD(CMUT3Device, Q_PUBLIC, timingSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, rpmSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, spdSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_1Supported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_2Supported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mafSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, tpsSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, ectSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mapSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, actSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, bapSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, vbatSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, fpwSupported, event_bool_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, wgdcSupported, event_bool_type(), ""),

		 XFIELD(CMUT3Device, Q_PUBLIC, timingMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, rpmMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, spdMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_1Min, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_2Min, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mafMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, tpsMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, ectMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mapMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, actMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, bapMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, vbatMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, fpwMin, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, wgdcMin, event_float_type(), ""),

		 XFIELD(CMUT3Device, Q_PUBLIC, timingMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, rpmMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, spdMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_1Max, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, lam_2Max, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mafMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, tpsMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, ectMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, mapMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, actMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, bapMax, event_float_type(), ""), 
		 XFIELD(CMUT3Device, Q_PUBLIC, vbatMax, event_float_type(), ""), 
		 XFIELD(CMUT3Device, Q_PUBLIC, fpwMax, event_float_type(), ""),
		 XFIELD(CMUT3Device, Q_PUBLIC, wgdcMax, event_float_type(), "")
		 );
public:
    CMUT3Device(CExecutor* aExec,
		CBaseType *aType = CMUT3DeviceType::singleton());
    ~CMUT3Device(void);
    void start(CExecutor *aExec);
    void execute(CExecutor* aExec);

private:
    bool openPort(CExecutor *aExec, TimeStamp aTimesTamp);
    void closePort(CExecutor *aExec);
    bool advancePID(void);
    bool requestPID(CExecutor *aExec, int aIndex);
    bool completePortSetup(CExecutor *aExec); 
    bool setBaudrate(int aBaudrate); 
    bool setSerialState(bool aBreak, bool aRTS);
    bool resetFaultCounter(void); 
    bool increaseFaultCounter(void); 
    bool processStream(CExecutor* aExec);


private:
    struct CPID {
	CPID(CExecutor* aExec,
	     char *aName, 
	     CExecutable *aOwner, 
	     uchar aPID, // 
	     bool aInvert, // -255 - x
	     float aPreAdd,
	     float aMultiplier,
	     float aPostAdd,
	     float aMinValue,
	     float aMaxValue):
	    mName(aName),
	    mValue(aOwner),
	    mUsage(aOwner),
	    mPID(aPID),
	    mInvert(aInvert),
	    mPreAdd(aPreAdd),
	    mMultiplier(aMultiplier),
	    mPostAdd(aPostAdd),
	    mMinValue(aOwner),
	    mMaxValue(aOwner),
	    mSupported(aOwner) {
	    mUsage.putValue(aExec, 0);
	    mMinValue.putValue(aExec, aMinValue);
	    mMaxValue.putValue(aExec, aMaxValue);
	    mValue.putValue(aExec, aMaxValue); // Min?
	    mSupported.putValue(aExec, 0);
	}

	char *mName;           // For debugging purposes only. 
	EventFloat mValue; // Tied to the runtime system.
	EventSigned mUsage; // Tied to the runtime system. Will be set to the number of instruments using this PID.
	uchar mPID;   // The PID of the channel we want
	bool mInvert; // Invert value (255 - x) before doing calculation
	float mPreAdd; // sent value = ((value + add_before) * multiplier) + add_after
	float mMultiplier; 
	float mPostAdd;
	EventFloat mMinValue;
	EventFloat mMaxValue;
	EventSigned mSupported;
    };
    typedef list<CPID *> CPIDList;
    typedef list<CPID *>::iterator CPIDListIterator;
    
    // All PIDS supported by the OBD-II J1979 standard.
    CPID *mPIDs[MUT3_PID_COUNT];

    // Serial file descriptor
    int mDescriptor;

    //
    // The current pid we are awaiting data from.
    // If mCurrentPID == -1, no channels are in use
    // and we are dormant even if we are still connected
    // to the ECU.
    //
    int mCurrentPID; 

    //
    // Counter keeping track of the number of failed attempts
    // we have in retrieving a pid from the ECU
    //
    int mFaultCount; 

    //
    // Port or file we are to read data from
    //
    EventString mPort;

    //
    // The poll revents variable subscribing to the mFileSource
    //
    EventSigned mRevents;
    EventSigned  mState;

    //
    // File descriptor object that ties
    // mDescriptor into the CSweeper
    // poll/scheduler system
    //
    CFileSource *mSource;

    //
    // Last time we tried to reopen a failed port
    //
    TimeStamp mLastOpenAttempt;

    //
    // Port reopen attempt timer.
    //
    CTimeout *mPortReopenTimer;
    EventBool mPortReopenTimeout;
 
    //
    // Read timeout timer
    //
    CTimeout *mReadTimeoutTimer;
    EventBool mReadTimeout;

    //
    // Port setup timer used to set break for 1.8 sek
    //
    CTimeout *mPortSetupTimeoutTimer;
    EventBool mPortSetupTimeout;
};

#endif // __MUT3_DEVICE_HH__

