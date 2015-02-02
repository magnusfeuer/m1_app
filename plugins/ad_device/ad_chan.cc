//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2008.
//

// 
// Magden propreitary AD converter protocol. Single channel
//
#include "ad_dev.hh"



XOBJECT_TYPE_BOOTSTRAP(CADChannel);

CADChannel::CADChannel(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDevice(0),
    mChannelIndex(-1),
    mValue(this),
    mTimeStamp(this),
    mSampleInterval(this), 
    mPullupResistor(this) 
{
    mValue.putValue(aExec, 0);
    mTimeStamp.putValue(aExec, 0);
    mSampleInterval.putValue(aExec, 0);
    mPullupResistor.putValue(aExec, 0);

    eventPut(aExec, XINDEX(CADChannel, value), &mValue);
    eventPut(aExec, XINDEX(CADChannel, timeStamp), &mTimeStamp);
    eventPut(aExec, XINDEX(CADChannel, sampleInterval), &mSampleInterval);
    eventPut(aExec, XINDEX(CADChannel, pullupResistor), &mPullupResistor);
}


CADChannel::~CADChannel(void) 
{
}

void CADChannel::execute(CExecutor* aExec) 
{ 
    if (mSampleInterval.updated()) {
	if (mChannelIndex == -1 || mDevice == 0) {
	    DBGFMT_CLS("CADChannel[%d]::execute(): Could not execute CADDevice[%p]. This is ok.", 
			mChannelIndex, mDevice);
	    return;
	}
	mDevice->setSampleInterval(mChannelIndex, mSampleInterval.value());
	return;
    }

    return;
}

void CADChannel::setup(CADDevice *aDevice, int aIndex) 
{
    mDevice = aDevice; 

    if (aIndex >= 0 && aIndex <= 15)
	mChannelIndex = aIndex;
}

unsigned int CADChannel::adValue(void) 
{
    return mValue.value(); 
}


void CADChannel::adValue(CExecutor *aExec, unsigned int aValue) 
{
    mValue.putValue(aExec, aValue); 
}




Time CADChannel::timeStamp(void) 
{
    return mTimeStamp.value(); 
}


void CADChannel::timeStamp(CExecutor *aExec, unsigned int aValue) 
{
    mTimeStamp.putValue(aExec, aValue); 
}

unsigned int CADChannel::sampleInterval(void) 
{
    return mSampleInterval.value(); 
}



unsigned int CADChannel::pullupResistor(void) 
{
    return mPullupResistor.value(); 
}


void CADChannel::setPullupResistor(CExecutor *aExec, unsigned int aValue) 
{
    mPullupResistor.putValue(aExec, aValue); 
}
