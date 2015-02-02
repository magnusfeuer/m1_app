//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "log.hh"
#include <string.h>
#include <unistd.h>
XOBJECT_TYPE_BOOTSTRAP(CLogReadChannel);
XOBJECT_TYPE_BOOTSTRAP(CLogWriteChannel);

CLogReadChannel::CLogReadChannel(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mName(this),
    mUnitType(this),
    mMinValue(0.0),
    mMaxValue(0.0),
    mInitialized(false)
{ 
    mName.putValue(aExec, "");
    mUnitType.putValue(aExec, "");
    eventPut(aExec, XINDEX(CLogReadChannel, name), &mName);  
    eventPut(aExec, XINDEX(CLogReadChannel, unitType), &mUnitType);  
    put(aExec, 
	XINDEX(CLogReadChannel, sampler), 
	UObject(
	    m1New(
		CSamplerBase, aExec, CSamplerBase::CSamplerBaseType::singleton()
		)
	    )
	);
}

void CLogReadChannel::execute(CExecutor* aExec)
{
}

void CLogReadChannel::addSample(CExecutor* aExec,float aValue, Time aTimeStamp)
{
    CSamplerBase*    sampler;

    if ((sampler = (CSamplerBase*) at(XINDEX(CLogReadChannel, sampler)).o) == NULL) {
	WARNFMT("LogReadChannel::addSample(): No sampler provided.");
	return;
    }

//     printf("LogReadChannel(%s): Adding [%f] at [%u]\n",
// 	   name(), aValue, aTimeStamp);
	
    //
    // Check if we need to add a first samle at start
    // of time line
    //
    if (!sampler->sampleCount() && aTimeStamp != 0) 
	sampler->addSample(aExec, aValue, 0);

    sampler->addSample(aExec, aValue, aTimeStamp);
    
    //
    // Update min/max of sampler?
    // Leave a 3% +- marginal at top and bottom
    //
    if (!mInitialized) {
	mMinValue = aValue;
	mMaxValue = aValue;

	if (mMinValue < 0.0)
	    sampler->minValue(mMinValue * 1.03);
	else
	    sampler->minValue(mMinValue * 0.97);


	if (mMaxValue < 0.0)
	    sampler->maxValue(mMaxValue * 0.97);
	else
	    sampler->maxValue(mMaxValue * 1.03);

	mInitialized = true;
	return;
    }

    if (aValue < mMinValue) {
	mMinValue = aValue;
	if (mMinValue < 0.0)
	    sampler->minValue(mMinValue * 1.03);
	else
	    sampler->minValue(mMinValue * 0.97);
    }

    if (aValue > mMaxValue) {
	mMaxValue = aValue;

	if (mMaxValue < 0.0) 
	    sampler->maxValue(mMaxValue * 0.97);
	else 
	    sampler->maxValue(mMaxValue * 1.03);
    }

    return;
}



CLogWriteChannel::CLogWriteChannel(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mName(this),
    mValue(this),
    mTimeStamp(this),
    mUsage(this),
    mLogStartTS(0LL),
    mIndex(0),
    mChannelCount(0),
    mDescriptor(-1)
{ 
    mName.putValue(aExec, "");
    mValue.putValue(aExec, 0.0);
    mUsage.putValue(aExec,  0);
    eventPut(aExec, XINDEX(CLogWriteChannel, name), &mName);  
    eventPut(aExec, XINDEX(CLogWriteChannel, value), &mValue); 
    eventPut(aExec, XINDEX(CLogWriteChannel, timeStamp), &mTimeStamp); 
    eventPut(aExec, XINDEX(CLogWriteChannel, usage), &mUsage); 
}

void CLogWriteChannel::setValue(CExecutor* aExec, float aValue) 
{ 
    mValue.putValue(aExec, aValue); 
}

void CLogWriteChannel::setTimeStamp(CExecutor* aExec, Time aTimeStamp) 
{ 
    mTimeStamp.putValue(aExec, aTimeStamp); 
}

void CLogWriteChannel::setDescriptor(int aDescriptor) 
{
    DBGFMT("CLogWriteChannel(%s): %s", 
	   mName.value().c_str(), 
	   (aDescriptor == -1)?"Deactivating":"Activating");

    mDescriptor = aDescriptor;
    log(0); // Initial record!
}


void CLogWriteChannel::log(Time aTimeStamp)
{
    char line[256];
    int comma_count = 0;
    const char *commas = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";

//     printf("CLogWriteChannel(%s): Value[%f]\n", 
// 	   mName.value().c_str(), 
// 	   mValue.value());

    // Check how many commas we are to output.
    
    sprintf(line, "%lu%.*s,%f%.*s\r\n", 
	    aTimeStamp, // msec since log start.
	    mIndex,
	    commas, 
	    mValue.value(),
	    mChannelCount - mIndex - 1,
	    commas);

    write(mDescriptor, line, strlen(line));
}


void CLogWriteChannel::execute(CExecutor* aExec)
{

    if (mValue.updated() && mDescriptor != -1 && mName.value() != "") {
	if (mTimeStamp.updated() && mTimeStamp.value() != 0)
	    log(mTimeStamp.value());
	else
	    log((aExec->cycleTime() - mLogStartTS) / 1000);
    }
    return;
}

