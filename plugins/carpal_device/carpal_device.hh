//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
// Car-Pal BlueTooth OBD-II driver
//
#ifndef __CARPAL_DEVICE_HH__
#define __CARPAL_DEVICE_HH__

#include <stdio.h>
#include "component.hh"
#include "time_sensor.hh"

typedef unsigned char uchar;

typedef enum { 
    UNKNOWN        = 0x0000, 
    ISO_9141_0808  = 0x0001, 
    ISO_9141_9494  = 0x0002, 
    ISO_14230_SLOW = 0x0004, // == KWP2000
    ISO_14230_FAST = 0x0008, // == KWP2000
    J1850_PWM      = 0x0010,
    J1850_VPW      = 0x0020,
    CAN_11BIT_250K = 0x0040,
    CAN_11BIT_500K = 0x0080,
    CAN_29BIT_500K = 0x0100,
    CAN_29BIT_250K = 0x0200,
    SAE_J1939      = 0x0400,
    KW_1281        = 0x0800,
    KW_82          = 0x1000
} CCarPalProtocol;

ENUM_TYPE(CCarPalProtocol, "CarPalProtocol",
	  ENUMERATION(unknown,          UNKNOWN),
	  ENUMERATION(iso_9141_0808,    ISO_9141_0808),
	  ENUMERATION(iso_9141_9494,    ISO_9141_9494),
	  ENUMERATION(iso_14230_slow,   ISO_14230_SLOW),
	  ENUMERATION(iso_14230_fast,   ISO_14230_FAST),
	  ENUMERATION(j1850_pwm,        J1850_PWM),
	  ENUMERATION(j1850_vpw,        J1850_VPW),
	  ENUMERATION(can_11bit_500kb,  CAN_11BIT_500K),
	  ENUMERATION(can_29bit_500kb,  CAN_29BIT_500K),
	  ENUMERATION(can_11bit_250kb,  CAN_11BIT_250K),
	  ENUMERATION(can_29bit_250kb,  CAN_29BIT_250K)
    );

//
// Run state 
//
#define UNDEFINED         0
#define NO_CONNECTION     1
#define QUERYING_PROTOCOL 2
#define RUNNING           3  // Compatible with apex
#define QUERYING_PIDS     4

ENUM_TYPE(CCarPalState, "CarPalState",
	  ENUMERATION(undefined,          UNDEFINED),
	  ENUMERATION(no_connection,      NO_CONNECTION),
	  ENUMERATION(querying_protocol,  QUERYING_PROTOCOL),
	  ENUMERATION(querying_pidsl,     QUERYING_PIDS),
	  ENUMERATION(running,            RUNNING)
    );

#define CARPAL_INPUT_BUFFER_SIZE 1024

// REQUEST_GENERIC_COMMAND
//  Command 1=Command  (00 Pids supported - 04 coolant temp etc )
//          2= <frame>  gets the freeze frame stored in ECU
//          3= <no-pid> Get stored error codes
//          4= Clears the DTC stored

#define REQUEST_GENERIC_COMMAND      2
#define REQUEST_SERIAL_NUMBER        3
#define REQUEST_VERSION_NUMBER       4
#define REQUEST_FOUND_PROTOCOL       5
#define REQUEST_CHIP_IDENT           6
#define REQUEST_CONNECT_ECU          7
#define REQUEST_DISCONNECT_ECU       8
#define REQUEST_KET_KEYWORDS         9
#define REQUEST_KLINE_MONITORING     10
#define REQUEST_SEND_ISO_9141        12
#define REQUEST_SEND_ISO_14230_SLOW  13
#define REQUEST_SEND_ISO_14230_FAST  18
#define REQUEST_KEEP_ALIVE           19  // <KeepISO>, <keepKWP>
#define REQUEST_SEND_J1850_PWM       24
#define REQUEST_SEND_J1850_VPW       25
#define REQUEST_SEND_CAN_11BIT_250   33
#define REQUEST_SEND_CAN_11BIT_500   34
#define REQUEST_SEND_CAN_29BIT_250   35
#define REQUEST_SEND_CAN_29BIT_500   36

#define RESPONSE_SERIAL_NUMBER       3   // 03 HB LB  - 3 bytes (serial)
#define RESPONSE_VERSION_NUMBER      4   // 04 HB LB  - 3 bytes (version)
#define RESPONSE_FOUND_PROTOCOL      5   // 05 HB LB  - 3 bytes (protocol)
#define RESPONSE_CHIP_IDENT          6   // 06 HB LB  - 3 bytes (identification)
#define RESPONSE_CONNECT_ECU         7   // 07 HB LB  - 3 bytes (protocol)
#define RESPONSE_DISCONNECT_ECU      8   // 08 HB LB  - 3 bytes (protocol)
#define RESPONSE_KET_KEYWORDS        9   // 09 KW1 KW2


// Number of entries in the carpal_pid[] table
#define CARPAL_PID_COUNT 21

class CCarPalDevice: public CExecutable {
public:
    XOBJECT_TYPE(CCarPalDevice, "CarPalDevice", "Car-Pal driver",
		 (CCarPalDevice_port,
		  CCarPalDevice_protocol,
		  CCarPalDevice_state,
		  CCarPalDevice_iso_gap_time,
		  CCarPalDevice_revents,
		  CCarPalDevice_port_reopen_timeout,
		  CCarPalDevice_read_timeout,
		  CCarPalDevice_latency,

		  CCarPalDevice_s_trim_1,
		  CCarPalDevice_l_trim_1,
		  CCarPalDevice_s_trim_2,
		  CCarPalDevice_l_trim_2,
		  CCarPalDevice_timing,
		  CCarPalDevice_rpm,
		  CCarPalDevice_spd,
		  CCarPalDevice_lam_1,
		  CCarPalDevice_lam_2,
		  CCarPalDevice_lam_3,
		  CCarPalDevice_lam_4,
		  CCarPalDevice_maf,
		  CCarPalDevice_load,
		  CCarPalDevice_tps,
		  CCarPalDevice_ect,
		  CCarPalDevice_fp,
		  CCarPalDevice_map,
		  CCarPalDevice_act,
		  CCarPalDevice_fuel,
		  CCarPalDevice_bap,
		  CCarPalDevice_vbat,

		  CCarPalDevice_s_trim_1Usage,
		  CCarPalDevice_l_trim_1Usage,
		  CCarPalDevice_s_trim_2Usage,
		  CCarPalDevice_l_trim_2Usage,
		  CCarPalDevice_timingUsage,
		  CCarPalDevice_rpmUsage,
		  CCarPalDevice_spdUsage,
		  CCarPalDevice_lam_1Usage,
		  CCarPalDevice_lam_2Usage,
		  CCarPalDevice_lam_3Usage,
		  CCarPalDevice_lam_4Usage,
		  CCarPalDevice_mafUsage,
		  CCarPalDevice_loadUsage,
		  CCarPalDevice_tpsUsage,
		  CCarPalDevice_ectUsage,
		  CCarPalDevice_fpUsage,
		  CCarPalDevice_mapUsage,
		  CCarPalDevice_actUsage,
		  CCarPalDevice_fuelUsage,
		  CCarPalDevice_bapUsage,
		  CCarPalDevice_vbatUsage,

		  CCarPalDevice_s_trim_1Supported,
		  CCarPalDevice_l_trim_1Supported,
		  CCarPalDevice_s_trim_2Supported,
		  CCarPalDevice_l_trim_2Supported,
		  CCarPalDevice_timingSupported,
		  CCarPalDevice_rpmSupported,
		  CCarPalDevice_spdSupported,
		  CCarPalDevice_lam_1Supported,
		  CCarPalDevice_lam_2Supported,
		  CCarPalDevice_lam_3Supported,
		  CCarPalDevice_lam_4Supported,
		  CCarPalDevice_mafSupported,
		  CCarPalDevice_loadSupported,
		  CCarPalDevice_tpsSupported,
		  CCarPalDevice_ectSupported,
		  CCarPalDevice_fpSupported,
		  CCarPalDevice_mapSupported,
		  CCarPalDevice_actSupported,
		  CCarPalDevice_fuelSupported,
		  CCarPalDevice_bapSupported,
		  CCarPalDevice_vbatSupported,

		  CCarPalDevice_s_trim_1Min,
		  CCarPalDevice_l_trim_1Min,
		  CCarPalDevice_s_trim_2Min,
		  CCarPalDevice_l_trim_2Min,
		  CCarPalDevice_timingMin,
		  CCarPalDevice_rpmMin,
		  CCarPalDevice_spdMin,
		  CCarPalDevice_lam_1Min,
		  CCarPalDevice_lam_2Min,
		  CCarPalDevice_lam_3Min,
		  CCarPalDevice_lam_4Min,
		  CCarPalDevice_mafMin,
		  CCarPalDevice_loadMin,
		  CCarPalDevice_tpsMin,
		  CCarPalDevice_ectMin,
		  CCarPalDevice_fpMin,
		  CCarPalDevice_mapMin,
		  CCarPalDevice_actMin,
		  CCarPalDevice_fuelMin,
		  CCarPalDevice_bapMin,
		  CCarPalDevice_vbatMin,

		  CCarPalDevice_s_trim_1Max,
		  CCarPalDevice_l_trim_1Max,
		  CCarPalDevice_s_trim_2Max,
		  CCarPalDevice_l_trim_2Max,
		  CCarPalDevice_timingMax,
		  CCarPalDevice_rpmMax,
		  CCarPalDevice_spdMax,
		  CCarPalDevice_lam_1Max,
		  CCarPalDevice_lam_2Max,
		  CCarPalDevice_lam_3Max,
		  CCarPalDevice_lam_4Max,
		  CCarPalDevice_mafMax,
		  CCarPalDevice_loadMax,
		  CCarPalDevice_tpsMax,
		  CCarPalDevice_ectMax,
		  CCarPalDevice_fpMax,
		  CCarPalDevice_mapMax,
		  CCarPalDevice_actMax,
		  CCarPalDevice_fuelMax,
		  CCarPalDevice_bapMax,
		  CCarPalDevice_vbatMax
		  ),

		 XFIELD(CCarPalDevice, Q_PUBLIC, port, event_string_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, protocol, CEventType::create(CCarPalProtocolType::singleton(),E_INPUT | E_INOUT), ""),
		 //		 XFIELD(CCarPalDevice, Q_PUBLIC, state, CEventType::create(CCarPalStateType::singleton(), E_INOUT), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, state, event_signed_type(), "current state"),
		 XFIELD(CCarPalDevice, Q_PUBLIC, iso_gap_time, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, revents, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, port_reopen_timeout, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, read_timeout, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, latency, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_1, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_1, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_2, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_2, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, timing, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, rpm, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, spd, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_1, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_2, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_3, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_4, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, maf, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, load, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, tps, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, ect, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fp, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, map, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, act, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fuel, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, bap, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, vbat, event_float_type(), ""),

		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_1Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_1Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_2Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_2Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, timingUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, rpmUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, spdUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_1Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_2Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_3Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_4Usage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mafUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, loadUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, tpsUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, ectUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fpUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mapUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, actUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fuelUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, bapUsage, event_signed_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, vbatUsage, event_signed_type(), ""),

		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_1Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_1Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_2Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_2Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, timingSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, rpmSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, spdSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_1Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_2Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_3Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_4Supported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mafSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, loadSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, tpsSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, ectSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fpSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mapSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, actSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fuelSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, bapSupported, event_bool_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, vbatSupported, event_bool_type(), ""),

		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_1Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_1Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_2Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_2Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, timingMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, rpmMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, spdMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_1Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_2Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_3Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_4Min, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mafMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, loadMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, tpsMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, ectMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fpMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mapMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, actMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fuelMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, bapMin, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, vbatMin, event_float_type(), ""),

		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_1Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_1Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, s_trim_2Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, l_trim_2Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, timingMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, rpmMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, spdMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_1Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_2Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_3Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, lam_4Max, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mafMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, loadMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, tpsMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, ectMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fpMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, mapMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, actMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, fuelMax, event_float_type(), ""),
		 XFIELD(CCarPalDevice, Q_PUBLIC, bapMax, event_float_type(), ""), 
		 XFIELD(CCarPalDevice, Q_PUBLIC, vbatMax, event_float_type(), "")
		 );

public:
    CCarPalDevice(CExecutor* aExec,
		CBaseType *aType = CCarPalDeviceType::singleton());
    ~CCarPalDevice(void);
    void execute(CExecutor* aExec);

private:
    struct ProtocolDescriptor {
	u_int16_t mProtocol;   // Protocol number
	char *mName;           // Name of protocol
	int mResponseSize;     // Number of bytes returned from $01 request.
    };

    static ProtocolDescriptor mProtocols[];
    int mUsedProtocol; // index into mProtocols

    void openPort(CExecutor *aExec, TimeStamp aTimesTamp);
    void reOpenPort(CExecutor* aExec);

    bool getData(void); // Do a read into raw buffer. Do not process
    bool extractData(uchar *aBuffer, int aLength); // Read data from raw buffer only.
    bool recvGenericReply(uchar *aBuffer, int* aLength);
    int  availableData(void) { return mInputBufferLen; }
    void resetData(void)     { mInputBufferLen = 0; }

    bool installPID(CExecutor* aExec, unsigned char aPID);
    bool sendGenericCommand(CExecutor *aExec, uchar cmd, uchar arg);
    bool sendNextRequest(CExecutor *aExec, TimeStamp ts);
    bool processQueryProtocolResult(CExecutor* aExec, char *aResult);
    void setPID(CExecutor* aExec, uchar *aBuffer, int len);

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

    CPID *findPID(uchar pid);

    // All PIDS supported by the OBD-II J1979 standard.
    CPID *mPIDs[CARPAL_PID_COUNT];

    //
    // The PIDs found on the given vehicle are installed in mProperties
    // The 0-3 index indicates the priority level.
    //
    CPIDList mProperties[4];        // Different levels.
    CPIDListIterator mIterators[4]; // Iterator for each level.
    CPIDListIterator mCurrent;      // Current iterator.
    CPID*    mCurrentPID;           // Last transmitted pid request
    
    int mDescriptor;                // File descriptor

    EventString mPort;              // Device name

    EventSigned mRevents;           // Poll events

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
    // Port reopen attempt timer.
    //
    CTimeout *mPortReopenTimer;
    EventBool mPortReopenTimeout;

    //
    // Read timeout timer
    //
    CTimeout *mReadTimeoutTimer;
    EventBool mReadTimeout;

    // Input buffer
    uchar mInputBuffer[CARPAL_INPUT_BUFFER_SIZE];
    int mInputBufferLen;

    //
    // ECU response time in seconds
    //
    EventFloat  mLatency;

    //
    // Number of read failures (timeout or nulls) we get while waiting for data.
    //
    int mReadFailCount;
    //
    // Offset while in state QUERING_PIDS
    //
    int          mPidOffset;
};

#endif // __CARPAL_DEVICE_HH__

		 




