//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
//  Harrison R&D OBDScan Apex driver.
// Initially just USB support, but that may change.
//
#ifndef __APEX_DEVICE_HH__
#define __APEX_DEVICE_HH__

#include <stdio.h>
#include "component.hh"
#include "time_sensor.hh"

typedef enum { 
    UNKNOWN = 0, 
    J1850_PWM = 1,
    J1850_VPW = 2,
    ISO_9141 = 3, 
    ISO_14230 = 4, // == KWP2000
    CAN_11BIT_500K = 5,
    CAN_29BIT_500K = 6,
    CAN_11BIT_250K = 7,
    CAN_29BIT_250K = 8
} CApexProtocol;

ENUM_TYPE(CApexProtocol, "ApexProtocol",
	  ENUMERATION(unknown,          UNKNOWN),
	  ENUMERATION(j1850_pwm,        J1850_PWM),
	  ENUMERATION(j1850_vpw,        J1850_VPW),
	  ENUMERATION(iso_9141,         ISO_9141),
	  ENUMERATION(iso1_4230,        ISO_14230),
	  ENUMERATION(can_11bit_500kb,  CAN_11BIT_500K),
	  ENUMERATION(can_29bit_500kb,  CAN_29BIT_500K),
	  ENUMERATION(can_11bit_250kb,  CAN_11BIT_250K),
	  ENUMERATION(can_29bit_250kb,  CAN_29BIT_250K)
    );


//
// Run state 
//
#define APEX_UNDEFINED 0
#define APEX_NO_CONNECTION 1
#define APEX_QUERYING_PROTOCOL 2
#define APEX_RUNNING 3

typedef enum { 
    UNDEFINED = 0, // No device found
    NO_CONNECTION = 1, // No device found
    QUERYING_PROTOCOL = 2, // Checking protocol.
    RUNNING = 3, // Running.
} CApexPState;

ENUM_TYPE(CApexState, "ApexState",
	  ENUMERATION(undefined,          UNDEFINED),
	  ENUMERATION(no_connection,      NO_CONNECTION),
	  ENUMERATION(querying_protocol,  QUERYING_PROTOCOL),
	  ENUMERATION(running,            RUNNING)
    );

#define APEX_MAX_LINE_LEN 2048

// Number of entries in the apex_pid[] table
#define APEX_PID_COUNT 21


class CApexDevice: public CExecutable {
public:
    XOBJECT_TYPE(CApexDevice, "ApexDevice", "Apex driver",
		 (CApexDevice_port,
		  CApexDevice_protocol,
		  CApexDevice_state,
		  CApexDevice_iso_gap_time,
		  CApexDevice_revents,
		  CApexDevice_port_reopen_timeout,
		  CApexDevice_read_timeout,
		  CApexDevice_latency,

		  CApexDevice_s_trim_1,
		  CApexDevice_l_trim_1,
		  CApexDevice_s_trim_2,
		  CApexDevice_l_trim_2,
		  CApexDevice_timing,
		  CApexDevice_rpm,
		  CApexDevice_spd,
		  CApexDevice_lam_1,
		  CApexDevice_lam_2,
		  CApexDevice_lam_3,
		  CApexDevice_lam_4,
		  CApexDevice_maf,
		  CApexDevice_load,
		  CApexDevice_tps,
		  CApexDevice_ect,
		  CApexDevice_fp,
		  CApexDevice_map,
		  CApexDevice_act,
		  CApexDevice_fuel,
		  CApexDevice_bap,
		  CApexDevice_vbat,

		  CApexDevice_s_trim_1Usage,
		  CApexDevice_l_trim_1Usage,
		  CApexDevice_s_trim_2Usage,
		  CApexDevice_l_trim_2Usage,
		  CApexDevice_timingUsage,
		  CApexDevice_rpmUsage,
		  CApexDevice_spdUsage,
		  CApexDevice_lam_1Usage,
		  CApexDevice_lam_2Usage,
		  CApexDevice_lam_3Usage,
		  CApexDevice_lam_4Usage,
		  CApexDevice_mafUsage,
		  CApexDevice_loadUsage,
		  CApexDevice_tpsUsage,
		  CApexDevice_ectUsage,
		  CApexDevice_fpUsage,
		  CApexDevice_mapUsage,
		  CApexDevice_actUsage,
		  CApexDevice_fuelUsage,
		  CApexDevice_bapUsage,
		  CApexDevice_vbatUsage,

		  CApexDevice_s_trim_1Supported,
		  CApexDevice_l_trim_1Supported,
		  CApexDevice_s_trim_2Supported,
		  CApexDevice_l_trim_2Supported,
		  CApexDevice_timingSupported,
		  CApexDevice_rpmSupported,
		  CApexDevice_spdSupported,
		  CApexDevice_lam_1Supported,
		  CApexDevice_lam_2Supported,
		  CApexDevice_lam_3Supported,
		  CApexDevice_lam_4Supported,
		  CApexDevice_mafSupported,
		  CApexDevice_loadSupported,
		  CApexDevice_tpsSupported,
		  CApexDevice_ectSupported,
		  CApexDevice_fpSupported,
		  CApexDevice_mapSupported,
		  CApexDevice_actSupported,
		  CApexDevice_fuelSupported,
		  CApexDevice_bapSupported,
		  CApexDevice_vbatSupported,

		  CApexDevice_s_trim_1Min,
		  CApexDevice_l_trim_1Min,
		  CApexDevice_s_trim_2Min,
		  CApexDevice_l_trim_2Min,
		  CApexDevice_timingMin,
		  CApexDevice_rpmMin,
		  CApexDevice_spdMin,
		  CApexDevice_lam_1Min,
		  CApexDevice_lam_2Min,
		  CApexDevice_lam_3Min,
		  CApexDevice_lam_4Min,
		  CApexDevice_mafMin,
		  CApexDevice_loadMin,
		  CApexDevice_tpsMin,
		  CApexDevice_ectMin,
		  CApexDevice_fpMin,
		  CApexDevice_mapMin,
		  CApexDevice_actMin,
		  CApexDevice_fuelMin,
		  CApexDevice_bapMin,
		  CApexDevice_vbatMin,

		  CApexDevice_s_trim_1Max,
		  CApexDevice_l_trim_1Max,
		  CApexDevice_s_trim_2Max,
		  CApexDevice_l_trim_2Max,
		  CApexDevice_timingMax,
		  CApexDevice_rpmMax,
		  CApexDevice_spdMax,
		  CApexDevice_lam_1Max,
		  CApexDevice_lam_2Max,
		  CApexDevice_lam_3Max,
		  CApexDevice_lam_4Max,
		  CApexDevice_mafMax,
		  CApexDevice_loadMax,
		  CApexDevice_tpsMax,
		  CApexDevice_ectMax,
		  CApexDevice_fpMax,
		  CApexDevice_mapMax,
		  CApexDevice_actMax,
		  CApexDevice_fuelMax,
		  CApexDevice_bapMax,
		  CApexDevice_vbatMax
		  ),

		 XFIELD(CApexDevice, Q_PUBLIC, port, event_string_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, protocol, CEventType::create(CApexProtocolType::singleton(),E_INPUT | E_INOUT), ""),
		 //		 XFIELD(CApexDevice, Q_PUBLIC, state, CEventType::create(CApexStateType::singleton(), E_INOUT), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, state, event_signed_type(), "current state"),
		 XFIELD(CApexDevice, Q_PUBLIC, iso_gap_time, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, revents, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, port_reopen_timeout, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, read_timeout, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, latency, event_float_type(), ""),
	

		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_1, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_1, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_2, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_2, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, timing, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, rpm, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, spd, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_1, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_2, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_3, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_4, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, maf, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, load, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, tps, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, ect, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fp, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, map, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, act, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fuel, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, bap, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, vbat, event_float_type(), ""),

		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_1Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_1Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_2Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_2Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, timingUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, rpmUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, spdUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_1Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_2Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_3Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_4Usage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mafUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, loadUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, tpsUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, ectUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fpUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mapUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, actUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fuelUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, bapUsage, event_signed_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, vbatUsage, event_signed_type(), ""),

		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_1Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_1Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_2Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_2Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, timingSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, rpmSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, spdSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_1Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_2Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_3Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_4Supported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mafSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, loadSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, tpsSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, ectSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fpSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mapSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, actSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fuelSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, bapSupported, event_bool_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, vbatSupported, event_bool_type(), ""),

		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_1Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_1Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_2Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_2Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, timingMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, rpmMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, spdMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_1Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_2Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_3Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_4Min, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mafMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, loadMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, tpsMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, ectMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fpMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mapMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, actMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fuelMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, bapMin, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, vbatMin, event_float_type(), ""),

		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_1Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_1Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, s_trim_2Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, l_trim_2Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, timingMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, rpmMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, spdMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_1Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_2Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_3Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, lam_4Max, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mafMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, loadMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, tpsMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, ectMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fpMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, mapMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, actMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, fuelMax, event_float_type(), ""),
		 XFIELD(CApexDevice, Q_PUBLIC, bapMax, event_float_type(), ""), 
		 XFIELD(CApexDevice, Q_PUBLIC, vbatMax, event_float_type(), "")
		 );
public:
    CApexDevice(CExecutor* aExec,
		CBaseType *aType = CApexDeviceType::singleton());
    ~CApexDevice(void);
    void execute(CExecutor* aExec);

private:
    void openPort(CExecutor *aExec, TimeStamp aTimesTamp);
    int readLine(char *aBuffer, int aTimeout = -1); // APEX_MAX_LINE_LEN bytes max size.
    bool installPID(CExecutor* aExec, unsigned char aPID);
    bool sendNextRequest(CExecutor *aExec, TimeStamp ts);
    bool getSupportedPIDs(CExecutor* aExec);
    bool processQueryProtocolResult(CExecutor* aExec, char *aResult);
    
    //
    // Set the PID given in the two first bytes of the buffer
    // to the value in the consecutive 2 or 4 bytes of
    // the buffer, depending on the PID length.
    // Return number of bytes to advance aBuffer
    int setPID(CExecutor* aExec, char *aBuffer);

private:
    struct CPID {
	CPID(CExecutor* aExec,
	     int aLevel, 
	     char *aName, 
	     CExecutable *aOwner, 
	     unsigned char aPID,
	     int aSize,
	     int aPreAdd,
	     float aMultiplier,
	     int aPostAdd,
	     float aMinValue,
	     float aMaxValue):
	    mLevel(aLevel),
	    mName(aName),
	    mValue(aOwner),
	    mUsage(aOwner),
	    mPID(aPID),
	    mSize(aSize),
	    mPreAdd(aPreAdd),
	    mMultiplier(aMultiplier),
	    mPostAdd(aPostAdd),
	    mMinValue(aOwner),
	    mMaxValue(aOwner),
	    mSupported(aOwner) { 
	    mUsage.putValue(aExec, 0);
	    mMinValue.putValue(aExec, aMinValue);
	    mMaxValue.putValue(aExec, aMaxValue);
	    mValue.putValue(aExec, aMaxValue);
	    mSupported.putValue(aExec, 0);
	}
	
	int mLevel;            // Priority level.
	char *mName;           // For debugging purposes only. 
	EventFloat mValue; // Tied to the runtime system.
	EventSigned mUsage; // Tied to the runtime system. Will be set to true if this PID is used.
	unsigned char mPID;   // The PID .
	int mSize;       // 1 or 2 bytes of storage in frame. 
	int mPreAdd; // sent value = ((value + add_before) * multiplier) + add_after
	float mMultiplier; 
	int mPostAdd;
	EventFloat mMinValue;
	EventFloat mMaxValue;
	EventSigned mSupported; // Does the ECU support this PID?
    };
    typedef list<CPID *> CPIDList;
    typedef list<CPID *>::iterator CPIDListIterator;
 
    CPID *getNextPID(void);

    // Locate the PID based on the 8 bit hex value
    // stored in the two first bytes of aBuffer.
    CPID *findPID(char *aBuffer);

    // All PIDS supported by the OBD-II J1979 standard.
    CPID *mPIDs[APEX_PID_COUNT];

    //
    // The PIDs found on the given vehicle are insatlled in mProperties
    // The 0-3 index indicates the priority level.
    //
    CPIDList mProperties[4]; // Different levels.
    CPIDListIterator mIterators[4]; // Iterator for each level.
    CPIDListIterator mCurrent;   // Current iterator.
    // USB serial file descriptor
    int mDescriptor;

    //
    // Port or file we are to read data from
    //
    EventString mPort;
    //
    // The poll revents variable subscribing to the mFileSource
    //
    EventSigned mRevents;

    //
    // ISO gap timing
    //
    EventSigned mIsoGapTiming;
    bool mIsoGapTimingPending; // We have not yet set the new ISO gap timing.

    //
    // Protocol to be used.
    // Set to force, set to unknown to auto detect.
    //
    EventSigned  mProtocol;
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
    // ECU response time in seconds
    //
    EventFloat  mLatency;

    //
    // Number of read failures (timeout or nulls) we get while waiting for data.
    //
    int mReadFailCount;
};

#endif // __APEX_DEVICE_HH__

