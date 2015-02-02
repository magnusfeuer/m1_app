//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//

// 
// PI_SYS3 device reader.
//

#include "pi_sys3_device.hh"
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

XOBJECT_TYPE_BOOTSTRAP(CPiSys3Device);

CPiSys3Device::CPiSys3Device(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDescriptor(-1),
    mFileInput(false),
    mStorageBufLength(0),
    mSource(NULL),
    mPort(this), 
    mReadInterval(this),
    mLastReadTS(0),
    mRevents(this),
    mRpm(this),
    mTps(this),
    mMap(this),
    mLam1(this),
    mLam1t(this),
    mLam2(this),
    mLam2t(this),
    mSpd(this),
    mDetg(this),
    mTj(this),
    mSa(this),
    mEop(this),
    mFp(this),
    mGpos(this),
    mVbat(this),
    mErr(this),
    mAct(this),
    mEct(this),
    mEot(this),
    mFt(this),
    mBap(this),
    mFc(this),
    mTex(this),
    mDeti(this),
    mCls(this),
    mSp1(this),
    mSp2(this),
    mSp3(this),
    mSp4(this),
    mSp5(this),
    mSp6(this),
    mSp7(this),
    mSp8(this) { 
    Frame template_frame_map[10][8] = {
	{ 
	    { 0,   2, 0, 1.0, 0 },  
	    { 0,   1, 0, 0.5, 0 },
	    { 0,   2, 0, 1.0, 0},
	    { 0,   1, 0, 0.01, 0 },
	    { 0, 1, 0, 0.78125, 0 },
	    { 0,  1, 0, 0.01, 0 },
	    { 0, 1, 0, 0.78125, 0},
	    { 0, 0, 0, 0.0, 0 }
	},

	{ 
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0,   2, 0, 0.004, 0},
	    { 0,   2, -360, 0.25, 0},
	    { 0, 0, 0, 0.0, 0 }
	},
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0, 1, 0, 50.0, 0 },
	    { 0, 1, 0, 50.0, 0 },
	    { 0, 1, 0, 1.0, 0 },
	    { 0, 1, 0, 1.0, 0 },
	    { 0, 0, 0, 0.0, 0 }
	}, 
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0, 1, 0, 0.1, 0 },
	    { 0,  1, 0, 1.0, 0 },
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 0, 0, 0.0, 0 }
	}, 
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 0, 0, 0.0, 0 }
	}, 
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0,  1, -55, 1.0, 0 },
	    { 0,  1, -55, 1.0, 0 },
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 0, 0, 0.0, 0 }

	}, 
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0,  1, -55, 1.0, 0 },
	    { 0,  1, 0, 1.0, 0 },
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 0, 0, 0.0, 0 }
	}, 
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0,  2, 0, 1, 0 },
	    { 0, 0, 0, 0.0, 0 }
	}, 
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0,   2, 0, 0.01, 0 },
	    { 0,  2, 0, 1, 0 },
	    { 0, 0, 0, 0.0, 0 }
	}, 
	{
	    { 0,  2, 0, 0.06, 0},
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 1, 0, 0.392157, 0},
	    { 0,  1, 0, 5.0, 0 },
	    { 0,  1, 0, 0.25, +32 },
	    { 0,  2, 0, 1.0, 0 },
	    { 0, 0, 0, 0.0, 0 }
	}
    };

    DBGFMT("CPiSys3Device::CPiSys3Device(): Called");
    mPort.putValue(aExec, ""); 
    mReadInterval.putValue(aExec,  200);
    mRpm.putValue(aExec,  0);
    mTps.putValue(aExec,  0);
    mMap.putValue(aExec,  0);
    mLam1.putValue(aExec,  0);
    mLam1t.putValue(aExec,  0);
    mLam2.putValue(aExec,  0);
    mLam2t.putValue(aExec,  0);
    mSpd.putValue(aExec,  0);
    mDetg.putValue(aExec,  0);
    mTj.putValue(aExec,  0);
    mSa.putValue(aExec,  0);
    mEop.putValue(aExec,  0);
    mFp.putValue(aExec,  0);
    mGpos.putValue(aExec,  0);
    mVbat.putValue(aExec,  13.24);
    mErr.putValue(aExec,  0);
    mAct.putValue(aExec,  0);
    mEct.putValue(aExec,  0);
    mEot.putValue(aExec,  0);
    mFt.putValue(aExec,  0);
    mBap.putValue(aExec,  0);
    mFc.putValue(aExec,  0);
    mTex.putValue(aExec,  0);
    mDeti.putValue(aExec,  0);
    mCls.putValue(aExec,  0);
    mSp1.putValue(aExec,  0);
    mSp2.putValue(aExec,  0);
    mSp3.putValue(aExec,  0);
    mSp4.putValue(aExec,  0);
    mSp5.putValue(aExec,  0);
    mSp6.putValue(aExec,  0);
    mSp7.putValue(aExec,  0);   
    mSp8.putValue(aExec,0);

    mSource = m1New(CFileSource, aExec);
    m1Retain(CFileSource, mSource);

    eventPut(aExec,XINDEX(CPiSys3Device,revents), &mRevents);  // RPM
    eventPut(aExec,XINDEX(CPiSys3Device,rpm), &mRpm);  // RPM
    eventPut(aExec,XINDEX(CPiSys3Device,tps), &mTps);    // Degrees
    eventPut(aExec,XINDEX(CPiSys3Device,map), &mMap);   // Millibar
    eventPut(aExec,XINDEX(CPiSys3Device,lam1), &mLam1);   // Lambda. Divide by 100 to get correct lambda.
    eventPut(aExec,XINDEX(CPiSys3Device,lam1t), &mLam1t );  // Lambda trim %.
    eventPut(aExec,XINDEX(CPiSys3Device,lam2), &mLam2);   // Lambda. Divide by 100 to get correct lambda.
    eventPut(aExec,XINDEX(CPiSys3Device,lam2t), &mLam2t);  // Lambda trim %.),
    eventPut(aExec,XINDEX(CPiSys3Device,spd), &mSpd);    // 0-300 kmh.
    eventPut(aExec,XINDEX(CPiSys3Device,detg), &mDetg);   // Detonation average
    eventPut(aExec,XINDEX(CPiSys3Device,tj), &mTj);      // Fuel pulse width.
    eventPut(aExec,XINDEX(CPiSys3Device,sa), &mSa);    // Ignition angle degrees BTDC.
    eventPut(aExec,XINDEX(CPiSys3Device,eop), &mEop);  // Oil pressure Millibar
    eventPut(aExec,XINDEX(CPiSys3Device,fp), &mFp);   // Fuel pressure.
    eventPut(aExec,XINDEX(CPiSys3Device,gpos), &mGpos);    // Gear position. R=0, N=1. first=2...
    eventPut(aExec,XINDEX(CPiSys3Device,vbat), &mVbat);    // Battery voltage.
    eventPut(aExec,XINDEX(CPiSys3Device,err), &mErr);  // Error flag.
    eventPut(aExec,XINDEX(CPiSys3Device,act), &mAct);  // Air temperature. Celcius.
    eventPut(aExec,XINDEX(CPiSys3Device,ect), &mEct);  // Water temperature.
    eventPut(aExec,XINDEX(CPiSys3Device,eot), &mEot);  // Oil temperature. C
    eventPut(aExec,XINDEX(CPiSys3Device,ft), &mFt);   // Fuel temperature. C
    eventPut(aExec,XINDEX(CPiSys3Device,bap), &mBap); // Barometric pressure. Mb.
    eventPut(aExec,XINDEX(CPiSys3Device,fc), &mFc);     // Fuel consumption
    eventPut(aExec,XINDEX(CPiSys3Device,tex), &mTex);   // Exhaust temperature. C
    eventPut(aExec,XINDEX(CPiSys3Device,deti), &mDeti);   // Det ignition correction. Degrees.
    eventPut(aExec,XINDEX(CPiSys3Device,cls), &mCls);  // Change light speed RPM.
    eventPut(aExec,XINDEX(CPiSys3Device,sp1), &mSp1);
    eventPut(aExec,XINDEX(CPiSys3Device,sp2), &mSp1);
    eventPut(aExec,XINDEX(CPiSys3Device,sp3), &mSp1);
    eventPut(aExec,XINDEX(CPiSys3Device,sp4), &mSp1);
    eventPut(aExec,XINDEX(CPiSys3Device,sp5), &mSp1);
    eventPut(aExec,XINDEX(CPiSys3Device,sp6), &mSp1);
    eventPut(aExec,XINDEX(CPiSys3Device,sp7), &mSp1);
    eventPut(aExec,XINDEX(CPiSys3Device,sp8), &mSp1); 
    eventPut(aExec,XINDEX(CPiSys3Device,port), &mPort);
    eventPut(aExec,XINDEX(CPiSys3Device,readInterval), &mReadInterval);

    //
    // Setup the frame map from template
    memcpy(mFrameMap, template_frame_map, sizeof(mFrameMap));

    mFrameMap[0][0].mSymbol = &mRpm;
    mFrameMap[0][1].mSymbol = &mTps;
    mFrameMap[0][2].mSymbol = &mMap;
    mFrameMap[0][3].mSymbol = &mLam1;
    mFrameMap[0][4].mSymbol = &mLam1t;
    mFrameMap[0][5].mSymbol = &mLam2;
    mFrameMap[0][6].mSymbol = &mLam2t;
    mFrameMap[0][7].mSymbol = 0;

    mFrameMap[1][0].mSymbol = &mSpd;
    mFrameMap[1][1].mSymbol = &mSp1;
    mFrameMap[1][2].mSymbol = &mDetg;
    mFrameMap[1][3].mSymbol = &mTj;
    mFrameMap[1][4].mSymbol = &mSa;
    mFrameMap[1][5].mSymbol = 0;


    mFrameMap[2][0].mSymbol = &mSpd;
    mFrameMap[2][1].mSymbol = &mSp1;
    mFrameMap[2][2].mSymbol = &mDetg;
    mFrameMap[2][3].mSymbol = &mEop;
    mFrameMap[2][4].mSymbol = &mFp;
    mFrameMap[2][5].mSymbol = &mGpos;
    mFrameMap[2][6].mSymbol = &mSp2;
    mFrameMap[2][7].mSymbol = 0;


    mFrameMap[3][0].mSymbol = &mSpd;
    mFrameMap[3][1].mSymbol = &mSp1;
    mFrameMap[3][2].mSymbol = &mDetg;
    mFrameMap[3][3].mSymbol = &mVbat;
    mFrameMap[3][4].mSymbol = &mSp4;
    mFrameMap[3][5].mSymbol = &mSp3;
    mFrameMap[3][6].mSymbol = 0;


    mFrameMap[4][0].mSymbol = &mSpd;
    mFrameMap[4][1].mSymbol = &mSp1;
    mFrameMap[4][2].mSymbol = &mDetg;
    mFrameMap[4][3].mSymbol = &mErr;
    mFrameMap[4][4].mSymbol = &mSp3;
    mFrameMap[4][5].mSymbol = 0;


    mFrameMap[5][0].mSymbol = &mSpd;
    mFrameMap[5][1].mSymbol = &mSp1;
    mFrameMap[5][2].mSymbol = &mDetg;
    mFrameMap[5][3].mSymbol = &mAct;
    mFrameMap[5][4].mSymbol = &mEct;
    mFrameMap[5][5].mSymbol = &mSp5;
    mFrameMap[5][6].mSymbol = 0;



    mFrameMap[6][0].mSymbol = &mSpd;
    mFrameMap[6][1].mSymbol = &mSp1;
    mFrameMap[6][2].mSymbol = &mDetg;
    mFrameMap[6][3].mSymbol = &mEot;
    mFrameMap[6][4].mSymbol = &mFt;
    mFrameMap[6][5].mSymbol = &mSp6;
    mFrameMap[6][6].mSymbol = 0;


    mFrameMap[7][0].mSymbol = &mSpd;
    mFrameMap[7][1].mSymbol = &mSp1;
    mFrameMap[7][2].mSymbol = &mDetg;
    mFrameMap[7][3].mSymbol = &mBap;
    mFrameMap[7][4].mSymbol = &mSp7;
    mFrameMap[7][5].mSymbol = 0;


    mFrameMap[8][0].mSymbol = &mSpd;
    mFrameMap[8][1].mSymbol = &mSp1;
    mFrameMap[8][2].mSymbol = &mDetg;
    mFrameMap[8][3].mSymbol = &mFc;
    mFrameMap[8][4].mSymbol = &mSp8;
    mFrameMap[8][5].mSymbol = 0;


    mFrameMap[9][0].mSymbol = &mSpd;
    mFrameMap[9][1].mSymbol = &mSp1;
    mFrameMap[9][2].mSymbol = &mDetg;
    mFrameMap[9][3].mSymbol = &mTex;
    mFrameMap[9][4].mSymbol = &mDeti;
    mFrameMap[9][5].mSymbol = &mCls;
    mFrameMap[9][6].mSymbol = 0;

}

CPiSys3Device::~CPiSys3Device(void)
{
    DBGFMT("CPiSys3Device::~CPiSys3Device(): Called");

    if (mDescriptor != -1)
	close(mDescriptor);
    m1Release(CFileSource, mSource);
}


void CPiSys3Device::execute(CExecutor* aExec) 
{
    TimeStamp tTimeStamp = aExec->cycleTime();
    int frame_ind = 0;
    int buf_ind = 0;
    int group_ind;
    bool changed = false;
    static unsigned char storage_buf[1024];
    static int storage_len = 0;
    unsigned char buf[1024];
    int len, read_res;
    int synch_start = 0;
    int retry;
    struct termios t;
    int twice = 1;

    if (mPort.updated()) {
	struct stat st;

	mFileInput = false;

	if (mDescriptor != -1)
	    close(mDescriptor);
	
	if (stat(mPort.value().c_str(), &st) < 0) {
	    DBGFMT_WARN("CPiSys3Device:configured(): Could not stat [%s]: %s",
			mPort.value().c_str(), strerror(errno));
	    return;
	}
	if (S_ISREG(st.st_mode)) {
	    if ((mDescriptor = open(mPort.value().c_str(), O_RDONLY)) < 0) {
		DBGFMT_WARN("CPiSys3Device:configured(): Could not open file [%s]: %s",
			    mPort.value().c_str(), strerror(errno));
		return;
	    }
	    mFileInput = true;
	}
	else {
	    if ((mDescriptor = open(mPort.value().c_str(), O_RDONLY)) < 0) {
		DBGFMT_WARN("CPiSys3Device:configured(): Could not open device [%s]: %s",
			    mPort.value().c_str(), strerror(errno));
		return;
	    }
	    //
	    // Setup device
	    //
	    memset(&t, 0, sizeof(t));  // valgrind complaint not for real !

	    tcgetattr(mDescriptor, &t);
	    t.c_iflag = IGNBRK | IGNPAR;
	    t.c_oflag = 0;

	    t.c_cflag = CS8 | CLOCAL | CREAD;
	    cfsetospeed(&t, B38400);
	    t.c_lflag = 0;
	    t.c_cc[VTIME] = 0;
	    t.c_cc[VMIN] = 12;
 
	    if (tcsetattr(mDescriptor, TCSANOW, &t) < 0) {
		DBGFMT("CPiSys3Device::configred(): could not set termio on [%s]: [%s].",
		       mPort.value().c_str(), strerror(errno));
		return;
	    }
	}
	// Setup the subscription model
	mSource->setDescriptor(aExec,mDescriptor);
	connect(XINDEX(CPiSys3Device,revents), mSource, XINDEX(CFileSource,revents));
	DBGFMT("CPiSys3Device::configured(): Done. Reading from [%s]", mPort.value().c_str());
	return;
    }

    while(twice--) {
	// The strategy:
	//  Read a shitload of data to empty the input buffer.
	//  Locate where the last synch byte is and process everything up to that.
	//  Move
	//  Read until we have ten frames. Ensure that we have a synch and frame id 0.
	//

    
	//
	// If we have file input, we need to pace ourselves
	//
 	if (mFileInput) {
 	    if (tTimeStamp - mLastReadTS >= mReadInterval.value())
 		mLastReadTS = tTimeStamp;
 	    else
 		return;
	}
	    
	//
	// First copy over any old crap we have from a previous read to the 
	// beginning of buf.
	//
	if (storage_len > 0) {
	    memcpy(buf, storage_buf, storage_len);
	}

	len = storage_len;

	if (mFileInput) 
	    read_res = read(mDescriptor, buf+len, 12-len);
	else
	    read_res = read(mDescriptor, buf+len, 1024-len);
	//
	// Check if we reached end of file, if we are reading from a file, that is.
	// If so, reset.

	//
	if (mFileInput) {
	    //
	    // Check if we reached end of file. If so, reset.
	    //
	    if (read_res < 12-len) {
		puts("Resetting file descriptor.");
		lseek(mDescriptor, 0, SEEK_SET);
		storage_len = 0;
		len = 0;
		read_res = read(mDescriptor, buf+len, 12-len);
	    }
	}
	len += read_res;
	//
	// Locate a descent start frame.
	//
	buf_ind = 0;
	while(buf_ind < len) {
	    if (buf[buf_ind] == 0xFF && 
		buf[buf_ind+1] < 10 &&
		(buf[buf_ind]+buf[buf_ind+1]+buf[buf_ind+2]+buf[buf_ind+3]+
		 buf[buf_ind+4]+buf[buf_ind+5]+buf[buf_ind+6]+
		 buf[buf_ind+7]+buf[buf_ind+8]+buf[buf_ind+9]+buf[buf_ind+10]) % 256 == buf[buf_ind+11])
		break;

	    if (buf[buf_ind] == 0xFF)
		DBGFMT("Nosynch buf[%d]=[0x%X] buf[%d]=[0x%X] buf[%d]=[%.3d]/[%.3d]. Flushing remaining and storage",
		       buf_ind,
		       buf[buf_ind], 
		       buf_ind+1,
		       buf[buf_ind + 1], 
		       buf_ind+11,
		       buf[buf_ind+11],
		       (buf[buf_ind]+buf[buf_ind+1]+buf[buf_ind+2]+buf[buf_ind+3]+
			buf[buf_ind+4]+buf[buf_ind+5]+buf[buf_ind+6]+
			buf[buf_ind+7]+buf[buf_ind+8]+buf[buf_ind+9]+buf[buf_ind+10]) % 256);
	    ++buf_ind;
	}

	if (buf_ind == len) {
	    DBGFMT("Found no legal frame within read data");
	    //
	    // Store for the next run
	    //
	    if (len > 1000) {
		int i;
		DBGFMT("FATAL: More than 1000 bytes in storage, but no frame!");
		for (i = 0; i < len; ++i)
		    DBGFMT("buf[%.3d]=[%.3d]", i, buf[i]);
		exit(0);
	    }
	    storage_len = len;
	    //    DBGFMT("Stroring a total of [%d] bytes for future scanning", storage_len);
	    memcpy(storage_buf, buf, storage_len);
	    return;
	}
  
	if (buf_ind > 0) 
	    DBGFMT("Resynched after %d bytes", buf_ind);

	synch_start = buf_ind;

	//
	// We are now standing at the first byte of legal data. Check the integrity of the rest of it.
	//
	while(1) {
	    if (buf_ind+12 > len) {
		//
		// Copy the remaining stuff to storage_buf for the next run.
		//
		storage_len = len - buf_ind;
		if (storage_len > 0) {
		    memcpy(storage_buf, &buf[buf_ind], storage_len);
		}
		len = buf_ind;
		break;
	    }
      
	    if (buf[buf_ind] == 0xFF && 
		buf[buf_ind+1] < 10 &&
		(buf[buf_ind]+buf[buf_ind+1]+buf[buf_ind+2]+buf[buf_ind+3]+
		 buf[buf_ind+4]+buf[buf_ind+5]+buf[buf_ind+6]+
		 buf[buf_ind+7]+buf[buf_ind+8]+buf[buf_ind+9]+buf[buf_ind+10]) % 256 == buf[buf_ind+11]) {
		buf_ind += 12;
		continue;
	    }
	    DBGFMT("Got corrupt data buf[0]=[0x%X] buf[1]=[0x%X] buf[11]=[0x%X]/[0x%X]. Flushing remaining and storage",
		   buf[buf_ind], buf[buf_ind + 1], buf[buf_ind+11],
		   (buf[buf_ind]+buf[buf_ind+1]+buf[buf_ind+2]+buf[buf_ind+3]+
		    buf[buf_ind+4]+buf[buf_ind+5]+buf[buf_ind+6]+
		    buf[buf_ind+7]+buf[buf_ind+8]+buf[buf_ind+9]+buf[buf_ind+10]) % 256);
	    len = buf_ind;
	    storage_len = 0;
	    break;
	}
    

	//
	// Loop through all  frames.
	//
	buf_ind = synch_start;
  
	while(buf_ind != len) {
	    Frame* fp;
	    //    DBGFMT("buf_ind[%d] len[%d] group_ind[%d]", buf_ind, len, buf[buf_ind + 1]);
	    buf_ind++;// Skip synch.
	    group_ind = buf[buf_ind]; // Record the frame we are at.
	    ++buf_ind; // Skip frame id.

	    //
	    // Traverse the current frame and extract all the data.
	    //
	    frame_ind = 0;
	    fp = &mFrameMap[group_ind][frame_ind];
	    while(fp->mSymbol) {
		unsigned short val;
		int res, current_val;

		//      DBGFMT("Updating property[%s]", mFrameMap[group_ind][frame_ind].mSymbol);
		//
		// Install property in two or one bytes depending on size.
		// 
		
		if (fp->mSize == 2) {
		    val = (buf[buf_ind] << 8) | buf[buf_ind + 1];
		    buf_ind+=2;
		} else {
		    val = buf[buf_ind];
		    ++buf_ind;
		}


		//    DBGFMT("buf_ind is now [%d]", buf_ind);
// 		if (group_ind == 0 && frame_ind == 3)
//  		    printf("CPiSys3Device: Frame[%d/%d] = %d - %f\n", group_ind, frame_ind,
// 			   val,
//  			   ((((float) (val+fp->mAddBefore))*fp->mMultiplier) + 
//  			    (float) fp->mAddAfter));

		fp->mSymbol->putValue(aExec, 
				      ((((float) (val+fp->mAddBefore))*fp->mMultiplier) + 
				       (float) fp->mAddAfter));
		++frame_ind;
		fp = &mFrameMap[group_ind][frame_ind];
		
	    }
	    ++buf_ind; // Checksum byte
	}
    }
    return;
}


