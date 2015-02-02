//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
// Consult 1 protocol, done for Nissan Skyline GT(S) 2001 for Fast and the Furious 4
//
#ifndef __CONSULT_DEVICE_HH__
#define __CONSULT_DEVICE_HH__

#include <stdio.h>
#include "component.hh"
#include "time_sensor.hh"

typedef unsigned char uchar;

//
// Run state 
//
typedef enum { 
    UNDEFINED = 0, // No device found
    NO_CONNECTION = 1, // No device found
    RUNNING = 3, // Running.
} CConsultState;

#define CONSULT_PID_COUNT 20
#define CONSULT_INPUT_BUFFER_SIZE 2048

ENUM_TYPE(CConsultState, "ConsultState",
	  ENUMERATION(undefined,          UNDEFINED),
	  ENUMERATION(no_connection,      NO_CONNECTION),
	  ENUMERATION(running,            RUNNING)
    );




class CConsultDevice: public CExecutable {
public:
    XOBJECT_TYPE(CConsultDevice, "ConsultDevice", "Consult driver",
		 (CConsultDevice_port,
		  CConsultDevice_protocol,
		  CConsultDevice_state,
		  CConsultDevice_iso_gap_time,
		  CConsultDevice_revents,
		  CConsultDevice_port_reopen_timeout,
		  CConsultDevice_read_timeout,
		  CConsultDevice_latency,

		  CConsultDevice_s_trim_1,
		  CConsultDevice_l_trim_1,
		  CConsultDevice_s_trim_2,
		  CConsultDevice_l_trim_2,
		  CConsultDevice_timing,
		  CConsultDevice_rpm,
		  CConsultDevice_spd,
		  CConsultDevice_lam_1,
		  CConsultDevice_lam_2,
		  CConsultDevice_maf,
		  CConsultDevice_load,
		  CConsultDevice_tps,
		  CConsultDevice_ect,
		  CConsultDevice_fp,
		  CConsultDevice_map,
		  CConsultDevice_act,
		  CConsultDevice_fuel,
		  CConsultDevice_bap,
		  CConsultDevice_vbat,
		  CConsultDevice_fpw, // fuel pulse width

		  CConsultDevice_s_trim_1Usage,
		  CConsultDevice_l_trim_1Usage,
		  CConsultDevice_s_trim_2Usage,
		  CConsultDevice_l_trim_2Usage,
		  CConsultDevice_timingUsage,
		  CConsultDevice_rpmUsage,
		  CConsultDevice_spdUsage,
		  CConsultDevice_lam_1Usage,
		  CConsultDevice_lam_2Usage,
		  CConsultDevice_mafUsage,
		  CConsultDevice_loadUsage,
		  CConsultDevice_tpsUsage,
		  CConsultDevice_ectUsage,
		  CConsultDevice_fpUsage,
		  CConsultDevice_mapUsage,
		  CConsultDevice_actUsage,
		  CConsultDevice_fuelUsage,
		  CConsultDevice_bapUsage,
		  CConsultDevice_vbatUsage,
		  CConsultDevice_fpwUsage,

		  CConsultDevice_s_trim_1Supported,
		  CConsultDevice_l_trim_1Supported,
		  CConsultDevice_s_trim_2Supported,
		  CConsultDevice_l_trim_2Supported,
		  CConsultDevice_timingSupported,
		  CConsultDevice_rpmSupported,
		  CConsultDevice_spdSupported,
		  CConsultDevice_lam_1Supported,
		  CConsultDevice_lam_2Supported,
		  CConsultDevice_mafSupported,
		  CConsultDevice_loadSupported,
		  CConsultDevice_tpsSupported,
		  CConsultDevice_ectSupported,
		  CConsultDevice_fpSupported,
		  CConsultDevice_mapSupported,
		  CConsultDevice_actSupported,
		  CConsultDevice_fuelSupported,
		  CConsultDevice_bapSupported,
		  CConsultDevice_vbatSupported,
		  CConsultDevice_fpwSupported,

		  CConsultDevice_s_trim_1Min,
		  CConsultDevice_l_trim_1Min,
		  CConsultDevice_s_trim_2Min,
		  CConsultDevice_l_trim_2Min,
		  CConsultDevice_timingMin,
		  CConsultDevice_rpmMin,
		  CConsultDevice_spdMin,
		  CConsultDevice_lam_1Min,
		  CConsultDevice_lam_2Min,
		  CConsultDevice_mafMin,
		  CConsultDevice_loadMin,
		  CConsultDevice_tpsMin,
		  CConsultDevice_ectMin,
		  CConsultDevice_fpMin,
		  CConsultDevice_mapMin,
		  CConsultDevice_actMin,
		  CConsultDevice_fuelMin,
		  CConsultDevice_bapMin,
		  CConsultDevice_vbatMin,
		  CConsultDevice_fpwMin,

		  CConsultDevice_s_trim_1Max,
		  CConsultDevice_l_trim_1Max,
		  CConsultDevice_s_trim_2Max,
		  CConsultDevice_l_trim_2Max,
		  CConsultDevice_timingMax,
		  CConsultDevice_rpmMax,
		  CConsultDevice_spdMax,
		  CConsultDevice_lam_1Max,
		  CConsultDevice_lam_2Max,
		  CConsultDevice_mafMax,
		  CConsultDevice_loadMax,
		  CConsultDevice_tpsMax,
		  CConsultDevice_ectMax,
		  CConsultDevice_fpMax,
		  CConsultDevice_mapMax,
		  CConsultDevice_actMax,
		  CConsultDevice_fuelMax,
		  CConsultDevice_bapMax,
		  CConsultDevice_vbatMax,
		  CConsultDevice_fpwMax
		  ),


		 XFIELD(CConsultDevice, Q_PUBLIC, port, event_string_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, state, CEventType::create(CConsultStateType::singleton(), E_INOUT), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, revents, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, port_reopen_timeout, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, read_timeout, event_bool_type(), ""),

		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_1, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_1, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_2, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_2, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, timing, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, rpm, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, spd, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_1, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_2, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, maf, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, load, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, tps, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, ect, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fp, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, map, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, act, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fuel, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, bap, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, vbat, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpw, event_float_type(), ""),

		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_1Usage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_1Usage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_2Usage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_2Usage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, timingUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, rpmUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, spdUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_1Usage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_2Usage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mafUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, loadUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, tpsUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, ectUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mapUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, actUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fuelUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, bapUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, vbatUsage, event_signed_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpwUsage, event_signed_type(), ""),

		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_1Supported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_1Supported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_2Supported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_2Supported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, timingSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, rpmSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, spdSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_1Supported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_2Supported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mafSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, loadSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, tpsSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, ectSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mapSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, actSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fuelSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, bapSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, vbatSupported, event_bool_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpwSupported, event_bool_type(), ""),

		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_1Min, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_1Min, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_2Min, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_2Min, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, timingMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, rpmMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, spdMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_1Min, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_2Min, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mafMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, loadMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, tpsMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, ectMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mapMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, actMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fuelMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, bapMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, vbatMin, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpwMin, event_float_type(), ""),

		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_1Max, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_1Max, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, s_trim_2Max, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, l_trim_2Max, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, timingMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, rpmMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, spdMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_1Max, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, lam_2Max, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mafMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, loadMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, tpsMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, ectMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fpMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, mapMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, actMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, fuelMax, event_float_type(), ""),
		 XFIELD(CConsultDevice, Q_PUBLIC, bapMax, event_float_type(), ""), 
		 XFIELD(CConsultDevice, Q_PUBLIC, vbatMax, event_float_type(), ""), 
		 XFIELD(CConsultDevice, Q_PUBLIC, fpwMax, event_float_type(), "")
		 );
public:
    CConsultDevice(CExecutor* aExec,
		CBaseType *aType = CConsultDeviceType::singleton());
    ~CConsultDevice(void);
    void execute(CExecutor* aExec);

private:
    void openPort(CExecutor *aExec, TimeStamp aTimesTamp);
    bool getSupportedPIDs(CExecutor* aExec);

private:
    struct CPID {
	CPID(CExecutor* aExec,
	     char *aName, 
	     CExecutable *aOwner, 
	     uchar aMSBReg, // ECU Register for MSB of value, 0xFF if not used
	     uchar aLSBReg, // ECU Register for LSB of value.
	     float aPreAdd,
	     float aMultiplier,
	     float aPostAdd,
	     float aMinValue,
	     float aMaxValue):
	    mName(aName),
	    mValue(aOwner),
	    mUsage(aOwner),
	    mMSBReg(aMSBReg),
	    mLSBReg(aLSBReg),
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
	uchar mMSBReg;   // The register containing the most significant byte of the value. 0xFF if single byte val.
	uchar mLSBReg;   // The register containing the least significant byte of teh value.
	float mPreAdd; // sent value = ((value + add_before) * multiplier) + add_after
	float mMultiplier; 
	float mPostAdd;
	EventFloat mMinValue;
	EventFloat mMaxValue;
	EventSigned mSupported;
    };
    typedef list<CPID *> CPIDList;
    typedef list<CPID *>::iterator CPIDListIterator;
 
    bool getData(void); // Do a read into raw buffer. Do not process
    int readData(uchar *aBuffer, int aLength, int mTimeout); // Read data from raw buffer or file descriptor.
    bool extractData(uchar *aBuffer, int aLength); // Read data from raw buffer only.
    int availableData(void) { return mInputBufferLen; }
    bool stopStreaming(void);
    bool setupStreamingProfile(void);
    bool processStream(CExecutor* aExec);
    
    // All PIDS supported by the OBD-II J1979 standard.
    CPID *mPIDs[CONSULT_PID_COUNT];
    // List of pids that are 
    // A) supported by the mPIDs list (mMSBReg != 0xFF || mLSBReg != 0xFF).
    // -AND-
    // B) has been reported as supported by the ECU.
    CPIDList mActivePIDs; 
    
    //
    // List of all PIDs currently being reported in streaming mode
    // They are listed in the same order as they are reported.
    //
    CPIDList mStreamingPIDs;

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

    // Input buffer
    uchar mInputBuffer[CONSULT_INPUT_BUFFER_SIZE];
    int mInputBufferLen;
};

#endif // __CONSULT_DEVICE_HH__

