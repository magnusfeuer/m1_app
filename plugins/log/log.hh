//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
// Generic logging device.
//
#ifndef __LOG__
#define __LOG__

#include "component.hh"
#include "sampler.hh"


class CLogReadChannel: public CExecutable {
public:
    XOBJECT_TYPE(CLogReadChannel, "LogReadChannel", "Log reader channel",
		 (CLogReadChannel_name,
		  CLogReadChannel_unitType,
		  CLogReadChannel_sampler),
		 
		 XFIELD(CLogReadChannel, Q_PUBLIC, name, event_string_type(), ""),
		 XFIELD(CLogReadChannel, Q_PUBLIC, unitType, event_string_type(), ""),
		 XFIELD(CLogReadChannel, Q_PUBLIC, sampler, CSamplerBase::CSamplerBaseType::singleton(), "")
		);
public:
    CLogReadChannel(CExecutor* aExec, 
		    CBaseType *aType = CLogReadChannelType::singleton());

    void addSample(CExecutor* aExec, float aValue, Time aTimeStamp);
    void execute(CExecutor* aExec);

    const char *name(void) { return mName.value().c_str(); }

    void setName(CExecutor* aExec, const char *aNewName) { 
	mName.putValue(aExec, aNewName); 
    }

    const char *unitType(void) { return mUnitType.value().c_str(); }

    void setUnitType(CExecutor* aExec, const char *aNewType) { 
	mUnitType.putValue(aExec, aNewType); 
    }

    CSamplerBase *sampler(void) { 
	return (CSamplerBase *) at(XINDEX(CLogReadChannel, sampler)).o; 
    }

private:
    //
    // Name 
    //
    EventString mName;

    //
    // unit type
    //
    EventString mUnitType;

    //

    // min value seen in addSample.
    //
    float mMinValue;

    //
    // mMaxValue value seen in addSample.
    //
    float mMaxValue;

    //
    // Initializer for min/max
    //
    bool mInitialized;
};


class CLogWriteChannel: public CExecutable {
public:
    XOBJECT_TYPE(CLogWriteChannel, "LogWriteChannel", "Log writer channel",
		(CLogWriteChannel_name,
		 CLogWriteChannel_usage,
		 CLogWriteChannel_timeStamp,
		 CLogWriteChannel_value),
		XFIELD(CLogWriteChannel, Q_PUBLIC, name, event_string_type(), ""),
		XFIELD(CLogWriteChannel, Q_PUBLIC, usage, event_signed_type(), ""),
		XFIELD(CLogWriteChannel, Q_PUBLIC, timeStamp, output_time_type(), ""),
		XFIELD(CLogWriteChannel, Q_PUBLIC, value, event_float_type(), "")
		);
public:
    CLogWriteChannel(CExecutor* aExec,
		     CBaseType *aType = CLogWriteChannelType::singleton());
    void execute(CExecutor* aExec);
    void setDescriptor(int aDescriptor);
    void setTimeStamp(CExecutor* aExec, Time aTimeStamp);
    void setValue(CExecutor* aExec, float aValue);
    void setStartTS(TimeStamp aStartTS) { 
	mLogStartTS = aStartTS; 
    }
    void setChannelIndex(int aIndex, int aChannelCount) {
	mIndex = aIndex; mChannelCount = aChannelCount; 
    }

    const char *name(void) { return mName.value().c_str(); }

    void setName (CExecutor* aExec,const char *aNewName) { 
	mName.putValue(aExec, aNewName);  
    }

    int usage(void) { return mUsage.value(); }
private:
    void log(Time aTimeStamp);

private:
    //
    // Name 
    //
    EventString mName;

    //
    // Value
    //
    EventFloat mValue;

    //
    // TimeStamp output
    //
    EventTime mTimeStamp;

    //
    // Usage
    //
    EventSigned mUsage;

    //
    // Start of log Timestamp
    //
    TimeStamp mLogStartTS;

    //
    // Our index in the channels array of our owning CLogRead/Write
    //
    int mIndex;

    //
    // Total number of Channel elements in our owning CLogRead/Write
    //
    int mChannelCount;

    //
    // Descriptor to write to.
    //
    int mDescriptor;
};



//
// File format is:
// TimeStamp,channel,value\n
//
// TimeStamp is milliseconds since start of logging.
// Channel is name of channel.
// Value is float value of channel
//
// An empty line indicates that a new log is started.
// First line contains the starting date YYYY-MM-DD HH:MM:SS
// Second line contains all the logged channel names, comma separated.
//
//
// Error codes:
//  0 - OK
//  1 - Could not open file.
//  2 - USB stick removed during logging.
//  3 - USB stick is full.
//
class CLogWriter: public CExecutable {
public:
    XOBJECT_TYPE(CLogWriter, "LogWriter", "Log writer class",
		(CLogWriter_channels,
		 CLogWriter_error,
		 CLogWriter_active,
		 CLogWriter_fileNamePattern,
		 CLogWriter_fileName),
		XFIELD(CLogWriter, Q_PUBLIC, channels, CArrayType::create(CLogWriteChannel::CLogWriteChannelType::singleton(), 0), ""),
		XFIELD(CLogWriter, Q_PUBLIC, error, event_signed_type(), ""),
		XFIELD(CLogWriter, Q_PUBLIC, active, event_bool_type(), ""),
		XFIELD(CLogWriter, Q_PUBLIC, fileNamePattern, event_string_type(), ""),  // %Y->Year %M ->Month %D->Day %h->hour %m->min %s->sec %d->seq_nr
		XFIELD(CLogWriter, Q_PUBLIC, fileName, event_string_type(), "")  // Actual file name used
		);
public:
    CLogWriter(CExecutor* aExec,
	       CBaseType *aType = CLogWriterType::singleton());
    ~CLogWriter(void);
    void execute(CExecutor* aExec);

private:
    void startLog(CExecutor* aExec, TimeStamp aTS);
    void stopLog(void);
    void propagateFileDescriptor(void);

private:
    //
    // output file descriptor that we write to.
    //
    int mDescriptor;

    //
    // Error code to set.
    //
    EventSigned mError;

    //
    // Active or not active
    //
    EventBool mActive;

    //
    // File name pattern.
    //
    EventString mFileNamePattern;

    //
    // Used file name.
    //
    EventString mFileName;

    //
    // Log start time stamp
    //
    TimeStamp mLogStartTS;

};


//
// CLogReaderer
//
class CLogReader: public CExecutable {
public:
    XOBJECT_TYPE(CLogReader, "LogReader", "Log file reader",
		 (CLogReader_channels,
		  CLogReader_startDate,
		  CLogReader_error,
		  CLogReader_state,
		  CLogReader_fileName),

		 XFIELD(CLogReader, Q_PUBLIC, channels, CArrayType::create(CLogReadChannel::CLogReadChannelType::singleton(), 0), ""),
		 XFIELD(CLogReader, Q_PUBLIC, startDate, event_string_type(), ""),
		 XFIELD(CLogReader, Q_PUBLIC, error, event_signed_type(), ""), // 
		 XFIELD(CLogReader, Q_PUBLIC, state, event_signed_type(), ""), // 0 = inactive,  1 = Read header, 2 = Header read, 3 = play
		 XFIELD(CLogReader, Q_PUBLIC, fileName, event_string_type(), "")  // %Y->Year %M ->Month %D->Day %h->hour %m->min %s->sec
		);
public:
    CLogReader(CExecutor* aExec,
	       CBaseType *aType = CLogReaderType::singleton());
    ~CLogReader(void);
    void execute(CExecutor* aExec);

private:
    int readLine(char *aBuffer, int aMaxLen);
    void closeReader(void);
    void setupReader(CExecutor* aExec);
    void readNextValue(CExecutor* aExec);
    void setStartTS(TimeStamp aStartTS) { mLogStartTS = aStartTS; }
    void addFinalSamples(CExecutor* aExec);

private:
    //
    // input file descriptor that we write to.
    //
    FILE *mIn;

    //
    // Error code to set.
    //
    EventSigned mError;

    //
    // File name
    //
    EventString mFileName;

    //
    // Bi directional state variable.
    //
    // If set to 0, log replay will cease.
    // If state is set to one from the outside, the header specified by
    //   fileName will be read, and the state will then be set to 2.
    // If state is 2 and is then set to 3, the log file will be played.
    //
    EventSigned mState;
    
    //
    // Internal state so that we know what our previous state
    // was when mState is updated
    //
    int mSavedState;

    //
    // Start date. Just the first line from the log file.
    //
    EventString mStartDate;

    //
    // Start of log Timestamp
    //
    TimeStamp mLogStartTS;
};

#endif // __LOG__

