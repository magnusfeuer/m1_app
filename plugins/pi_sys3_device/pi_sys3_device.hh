//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//

//
//  PI SYS 3 datastream device.
//
#include "component.hh"

#ifndef __PI_SYS3_DEVICE_H__
#define __PI_SYS3_DEVICE_H__


class CPiSys3Device: public CExecutable {
public:
    XOBJECT_TYPE(CPiSys3Device, 
		 "PiSys3Device", 
		 "Pi System 3",
		 (CPiSys3Device_rpm,
		  CPiSys3Device_tps,
		  CPiSys3Device_map,
		  CPiSys3Device_lam1,
		  CPiSys3Device_lam1t,
		  CPiSys3Device_lam2,
		  CPiSys3Device_lam2t,
		  CPiSys3Device_spd,
		  CPiSys3Device_detg,
		  CPiSys3Device_tj,
		  CPiSys3Device_sa,
		  CPiSys3Device_eop,
		  CPiSys3Device_fp,
		  CPiSys3Device_gpos,
		  CPiSys3Device_vbat,
		  CPiSys3Device_err,
		  CPiSys3Device_act,
		  CPiSys3Device_ect,
		  CPiSys3Device_eot,
		  CPiSys3Device_ft,
		  CPiSys3Device_bap,
		  CPiSys3Device_fc,
		  CPiSys3Device_tex,
		  CPiSys3Device_deti,
		  CPiSys3Device_cls,
		  CPiSys3Device_sp1,
		  CPiSys3Device_sp2,
		  CPiSys3Device_sp3,
		  CPiSys3Device_sp4,
		  CPiSys3Device_sp5,
		  CPiSys3Device_sp6,
		  CPiSys3Device_sp7,
		  CPiSys3Device_sp8,
		  CPiSys3Device_port,
		  CPiSys3Device_revents,
		  CPiSys3Device_readInterval),
		 XFIELD(CPiSys3Device,Q_PUBLIC, rpm, 
			event_float_type(), "RPM"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, tps,
			event_float_type(),
			"Degrees"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, map,
			event_float_type(),
			"Millibar"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, lam1,
			event_float_type(), "Lambda. 0-2"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, lam1t,
			event_float_type(),
			"Lambda trim %."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, lam2, 
			event_float_type(),
			"Lambda. 0-2"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, lam2t,
			event_float_type(),
			"Lambda trim %."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, spd, 
			event_float_type(),
			"0-300 kmh."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, detg,
			event_float_type(),
			"Detonation average"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, tj,
			event_float_type(),
			"Fuel pulse width."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sa, 
			event_float_type(),
			"Ignition angle degrees BTDC."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, eop, 
			event_float_type(),
			"Oil pressure Millibar"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, fp, 
			event_float_type(),
			"Fuel pressure."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, gpos,
			event_float_type(),
			"Gear position. R=0, N=1. first=2..."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, vbat,
			event_float_type(),
			"Battery voltage."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, err,
			event_float_type(),
			"Error flag."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, act, 
			event_float_type(),
			"Air temperature. Celcius."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, ect, 
			event_float_type(),
			"Water temperature."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, eot, 
			event_float_type(),
			"Oil temperature. C"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, ft, 
			event_float_type(),
			"Fuel temperature. C"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, bap,
			event_float_type(),
			"Barometric pressure. Mb."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, fc, 
			event_float_type(),
			"Fuel consumption."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, tex,
			event_float_type(),
			"Exhaust temperature. C"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, deti,
			event_float_type(),
			"Det ignition correction. Degrees."),
		 XFIELD(CPiSys3Device,Q_PUBLIC, cls, 
			event_float_type(),
			"Change light speed RPM"),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp1, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp2, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp3, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp4, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp5, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp6, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp7, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, sp8, 
			event_float_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, port,
			event_string_type(),
			""),
		 XFIELD(CPiSys3Device,Q_PUBLIC, revents,
			event_signed_type(),
			""),
		XFIELD(CPiSys3Device,Q_PUBLIC, readInterval,
		       event_signed_type(),
		       ""),
	);

public:
    CPiSys3Device(CExecutor* aExec,
		  CBaseType *aType = CPiSys3DeviceType::singleton());
    ~CPiSys3Device(void);
    void execute(CExecutor* aExec);

private:
    struct Frame {
	EventFloat *mSymbol;
	int mSize;       // 1 or 2 bytes of storage in frame. 
	int mAddBefore; // sent value = ((property + add_before) * multiplier) + add_after
	float mMultiplier;
	int mAddAfter;
    };
    //
    // Frame map descritbing all frames recevived over PI_SYS3 stream.
    // First index is frame group.
    // Second index is frame within that group.
    //
    Frame mFrameMap[10][8];

    //
    // File or device descriptor we are reading from.
    //
    int mDescriptor;
    
    // 
    //
    //
    bool mFileInput; // And not RS232 input.

    //
    // Stores read-aheads of incomplete frames for next cycle
    //
    unsigned char mStorageBuf[1024];

    //
    // Length of mStorageBuf
    //
    int mStorageBufLength;

    //
    // File descriptor object that ties
    // mDescriptor into the CSweeper
    // poll/scheduler system
    //
    CFileSource* mSource;

    //
    // Port or file we are to read data from
    //
    EventString mPort;

    //
    // (Optional)
    // milliseconds to sleep between each reads.
    // Used to pace reads from a file to limit speed.
    // Not used when data stream is read from device.
    //
    EventSigned mReadInterval; // Sleep after read. Speed limiter for file input

    //
    //  Timestamp to keep track of paced reading.
    //
    TimeStamp mLastReadTS;

    //
    // The poll revents variable subscribing to the mFileSource
    //
    EventSigned mRevents;

    // Data channels
    EventFloat mRpm;
    EventFloat mTps;
    EventFloat mMap;
    EventFloat mLam1;
    EventFloat mLam1t;
    EventFloat mLam2;
    EventFloat mLam2t;
    EventFloat mSpd;
    EventFloat mDetg;
    EventFloat mTj;
    EventFloat mSa;
    EventFloat mEop;
    EventFloat mFp;
    EventFloat mGpos;
    EventFloat mVbat;
    EventFloat mErr;
    EventFloat mAct;
    EventFloat mEct;
    EventFloat mEot;
    EventFloat mFt;
    EventFloat mBap;
    EventFloat mFc;
    EventFloat mTex;
    EventFloat mDeti;
    EventFloat mCls;
    EventFloat mSp1;
    EventFloat mSp2;
    EventFloat mSp3;
    EventFloat mSp4;
    EventFloat mSp5;
    EventFloat mSp6;
    EventFloat mSp7;
    EventFloat mSp8;
};


#endif // __PI_SYS3_DEVICE_H__


