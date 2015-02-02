//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "log.hh"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

XOBJECT_TYPE_BOOTSTRAP(CLogReader);

CLogReader::CLogReader(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mIn(0),
    mError(this),
    mFileName(this),
    mState(this),
    mSavedState(0),
    mStartDate(this),
    mLogStartTS(0LL)
{ 
    CArrayType* t  = CArrayType::create(CLogReadChannel::CLogReadChannelType::singleton(), 0);
    CArray *a = new CArray(aExec,t,sizeof(CLogReadChannel *), 0);

    mState.putValue(aExec, 0);
    mError.putValue(aExec, 0);
    mStartDate.putValue(aExec, "");

    put(aExec, XINDEX(CLogReader, channels), UArray(a));

    eventPut(aExec, XINDEX(CLogReader, startDate), &mStartDate); 
    eventPut(aExec, XINDEX(CLogReader, error), &mError); 
    eventPut(aExec, XINDEX(CLogReader, fileName), &mFileName); 
    eventPut(aExec, XINDEX(CLogReader, state), &mState); 
    setFlags(ExecuteEverySweep);
}


CLogReader::~CLogReader(void) 
{
    m1ReleaseArray(at(XINDEX(CLogReader, channels)).arr);
}

int CLogReader::readLine(char *aBuffer, int aMaxLen)
{
    int len;
    if (!fgets(aBuffer, aMaxLen, mIn))
	return -1;

    len = strlen(aBuffer);

    //
    // Strip cr/nl
    //
    while(aBuffer[len - 1] == '\r' ||  aBuffer[len - 1] == '\n') {
	aBuffer[len - 1] = 0;
	len--;
    }
    return len;
}

void CLogReader::closeReader(void)
{
    if (!mIn)
	return;

    fclose(mIn);
    mIn = 0;
 
    setFlags(ExecuteOnEventUpdate);

    // Rst scheduled values.
    return;
}


void CLogReader::setupReader(CExecutor* aExec)
{
    char buf[1024];
    char *tok;
    int ind = 0;
    if (!mIn) 
	return;

    //
    // Read the first line, which is the date.
    //
    if (readLine(buf, 1024) == -1) {
	DBGFMT_CLS("Failed to read first line. Setting state to 0\n");
	closeReader();
	mState.putValue(aExec, 0);
	return;
    }
    mStartDate.putValue(aExec, buf);

    // Second line contains all the logged channels
    if (readLine(buf, 1024) == -1) {
	closeReader();
	DBGFMT_CLS("Failed to read second line. Setting state to 0\n");
	mState.putValue(aExec, 0);
	return;
    }

    //
    // resize the array to the correct number of elements
    //
    tok = buf;
    ind = 0;
    while(*tok) {
	if (*tok == ',')
	    ++ind;
	++tok;
    }
    at(XINDEX(CLogReader, channels)).arr->resize(ind);

    //
    // Divide read line into tokens.
    //
    strtok(buf, ","); // First is msec, which we don't really care about channelwise.
    ind = 0;
    while((tok = strtok(0, ",")) != 0) {
	CLogReadChannel *chan = m1New(CLogReadChannel, aExec, CLogReadChannel::CLogReadChannelType::singleton());
	char *unitType;
	char tmp[512];

	// Make a copy that we can patch without messing up strtok.
	strncpy(tmp, tok, 512); tmp[511] = 0;
	if ((unitType = strchr(tmp, ':')) != 0) {
	    *unitType++ = 0;
	    chan->setName(aExec, tmp);
	    chan->setUnitType(aExec, unitType);
	} else // No unittype provided.
	    chan->setName(aExec, tmp);

	printf("Adding [%s] as a channel[%d]\n", tok, ind);
	at(XINDEX(CLogReader, channels)).arr->put(aExec, ind, UObject(chan));
	ind++;
    }
}

void CLogReader::addFinalSamples(CExecutor* aExec)
{
    CArray* channels = at(XINDEX(CLogReader, channels)).arr;
    Time max_time = 0;
    int ind;


    //
    // Find highest value timestamp.
    //
    ind = channels->size();
    while(ind--) {
	CLogReadChannel *chan = dynamic_cast<CLogReadChannel *>(channels->at(ind).o);
	CSamplerBase *sampler = chan->sampler();
	CSampleData *s_data;

	// Ignore channels with no samples.
	if (!sampler || sampler->sampleCount() == 0)
	    continue;
	
	printf("Max for %s [%f]\n", 
	       chan->name(), sampler->maxValue());
	s_data = sampler->sample(sampler->sampleCount() - 1);

	// Check if we have a new highest time.
	if (s_data && s_data->ts() > max_time)
	    max_time = s_data->ts();
    }	

    //
    // Add final sample at end of time to all channels
    // except the one ending with the highest timestamp
    //
    ind = channels->size();
    while(ind--) {
	CLogReadChannel *chan = dynamic_cast<CLogReadChannel *>(channels->at(ind).o);
	CSamplerBase *sampler = chan->sampler();
	CSampleData *s_data;

	// Ignore channels with no samples.
	if (sampler->sampleCount() == 0)
	    continue;
	
	s_data = sampler->sample(sampler->sampleCount() - 1);

	//
	// Check if we should add a final sample 
	// at the end of the time line
	//
	if (s_data && s_data->ts() != max_time)
	    chan->addSample(aExec, s_data->val(), max_time);
    }	

}


void CLogReader::readNextValue(CExecutor* aExec)
{
    char buf[1024];
    int ind = 0;
    char *start = buf;
    char *end;
    CArray* channels = at(XINDEX(CLogReader, channels)).arr;
    Time ts;
    float val;

    if (readLine(buf, 1024) == -1)  {
	DBGFMT_CLS("CLogReader::readNextValue(): Reached EOF. Setting state to 4\n");
	addFinalSamples(aExec);
	closeReader();
	mState.putValue(aExec, 4);
	return;
    }

    //
    // Grab timestamp for when to deliver this
    //
    if (!(end = strchr(start, ','))) {
	DBGFMT_CLS("Could not find a comma in [%s]\n", start);
	closeReader();
	mState.putValue(aExec, 0);
	return;
    }

    *end = 0;

    ts = atol(start);
//     printf("LogStart [%llu] start[%lu] Schedule [%llu]. TS[%llu]\n", 
// 	   mLogStartTS, atol(start), mScheduleTS, aTS);
	   
    // Check the number of commas
    start = end + 1;
    while(*start == ',') {
	++ind;
	++start;
    }

    //
    // We are standing at the first digit of the value. 
    // Validate
    //
    if ((*start < '0' || *start > '9') && *start != '.' && *start != '-') {
	DBGFMT_CLS("Not a digit[%s])\n", start);
	closeReader();
	mState.putValue(aExec, 0);
	return;
    }

    // Find end
    end = start;
    while((*end >= '0' && *end <= '9') || *end == '.' || *end == '-')
	++end;
    *end = 0;
    
    val = atof(start);
    
    //
    // Set scheduled channel, if we have it
    //
    if (ind >= channels->size()) {
	DBGFMT_CLS("ind[%d] >= channel->size(%d)\n", ind, channels->size());
	closeReader();
	mState.putValue(aExec, 0);
	return;
    }
    
	
    dynamic_cast<CLogReadChannel *>(channels->at(ind).o)->addSample(aExec,val,ts);


//     printf("Will set [%s] to [%f] at [%d] msec\n",
//   	   dynamic_cast<CLogReadChannel *>(channels->at(ind).o)->name(),
// 	   val,
// 	   ts);

    return;
}

void CLogReader::execute(CExecutor* aExec)
{
    int read_cnt;

    if (mFileName.updated()) {
	printf("CLogReader: Filename[%s]\n", mFileName.value().c_str());
	if (mIn)
	    fclose(mIn);

	//
	// Check if file name has been set to "".
	// If so, close the reader (if it is active).
	//
	if (mFileName.value() == "") {
	    closeReader();
	    DBGFMT_CLS("CLogReader::execute(): Filename is empty. Setting state to 0\n");
	    mState.putValue(aExec, 0);
	    mError.putValue(aExec, 2);
	    return;
	}
    }

    if (mState.updated()) {
	// Inactivate
	if (mState.value() == 0) {
	    DBGFMT_CLS("CLogReader::execute(): State is set to 0 externally.\n");
	    closeReader();
	    mSavedState = mState.value();
	    return;
	}

	//
	// Read header.
	//
	if (mState.value() == 1) {
//	    printf("State = 1\n");
	    if (mIn) 
		closeReader();

	    mIn = fopen(mFileName.value().c_str(), "r" );

	    if (!mIn) {
		perror(mFileName.value().c_str());
		mError.putValue(aExec, 1);
		mSavedState = 0;
		mState.putValue(aExec, 0);
		return;
	    }

	    setupReader(aExec);

	    mState.putValue(aExec, 2);
	    mSavedState = 2;
	    return;
	}

	if (mState.value() == 3) {
	    printf("LogState == 3\n");
	    if (mSavedState != 2) {
		printf("Previous logstate was not 2. Ignore.\n");
		return;
	    }	    
	    
	    // Read first line and schedule it for delivery to the system
	    mLogStartTS = aExec->cycleTime();
	}
    }


    // Read 1000 lines at the time, if we are in the right state.
    read_cnt = 1000;
    while(read_cnt-- && mState.value() == 3)
	readNextValue(aExec);

    return;
}
