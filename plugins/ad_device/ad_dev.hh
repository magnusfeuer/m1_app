//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2008.
//

//
// Device driver for Magden in-house A/D converter.
//

#ifndef __AD_DEV_HH__
#define __AD_DEV_HH__

#include "component.hh"
#include "time_sensor.hh"

#define WAIT_NONE 0   // Nostate
#define WAIT_INIT_START 10   // flush input and wait for version info(after sending \r\r\r\r\V))

typedef enum {
    AD_DEVICE_STATE_NONE   = 0,  // none
    AD_DEVICE_STATE_NODEV  = 1,  // could not open device, or no support
    AD_DEVICE_STATE_INIT   = 2,  // initialize
    AD_DEVICE_STATE_CLOSED = 3,  // ad device closed
    AD_DEVICE_STATE_OPEN   = 4   // ad device open
} EADDeviceState;

ENUM_TYPE(CADDeviceState, "ADDeviceState",
	  ENUMERATION(none, AD_DEVICE_STATE_NONE),
	  ENUMERATION(nodev, AD_DEVICE_STATE_NODEV),
	  ENUMERATION(init, AD_DEVICE_STATE_INIT),
	  ENUMERATION(closed, AD_DEVICE_STATE_CLOSED),
	  ENUMERATION(open, AD_DEVICE_STATE_OPEN)
    );

class CADDevice;

#define AD_INPUT_BUFFER_SIZE 10240

class CADChannel : public CExecutable {
public:
    XOBJECT_TYPE(CADChannel, "MagdenADChannel", "Single channel managed by an A/D device.",
	       (CADChannel_value, // Current sampled value.
		CADChannel_timeStamp, // Timestamp when value was last set (ms since sys start).
		CADChannel_sampleInterval, // Sample interval in msec.
		CADChannel_pullupResistor), // The pullup resistor. Matched against sensor profiles.
		 XFIELD(CADChannel,Q_PUBLIC, value, output_unsigned_type(), "Sampled value."),
		 XFIELD(CADChannel,Q_PUBLIC, timeStamp, output_time_type(), "Absolute timestamp when value was read."),
		 XFIELD(CADChannel,Q_PUBLIC, sampleInterval, input_unsigned_type(), "Interval, in msec, that value should be sampled."),
		 XFIELD(CADChannel,Q_PUBLIC, pullupResistor, event_unsigned_type(), "Pullup resistor tied to this channel.")
	);

public:
    CADChannel(CExecutor* aExec, CBaseType *aType = CADChannel::CADChannelType::singleton());

    ~CADChannel(void);

    void execute(CExecutor* aExec);
    void setup(CADDevice *aDevice, int aIndex);
    unsigned int adValue(void);
    void adValue(CExecutor *aExec, unsigned int aValue);

    Time timeStamp(void);
    void timeStamp(CExecutor *aExec, unsigned int aValue);

    unsigned int sampleInterval(void);
    void setSampleInterval(CExecutor *aExec, unsigned int aValue); 

    unsigned int pullupResistor(void);
    void setPullupResistor(CExecutor *aExec, unsigned int aValue); 


private:
    CADDevice *mDevice; // Device to route update requests through.
    int mChannelIndex;
    EventUnsigned mValue;
    EventTime mTimeStamp;
    EventUnsigned mSampleInterval;
    EventUnsigned mPullupResistor;
};



class CADDevice: public CExecutable {
public:
    XOBJECT_TYPE(CADDevice, 
		 "MagdenADDevice",
		 "Magden A/D converterdevice driver",
		 (CADDevice_revents,
		  CADDevice_state,
		  CADDevice_channels,
		  CADDevice_port,
		  CADDevice_portSpeed,
		  CADDevice_timeout),
		 XFIELD(CADDevice,Q_PUBLIC,revents, event_signed_type(), "device event input mask"),
		 XFIELD(CADDevice,Q_PUBLIC,port,
		       event_string_type(),
		       "AD device port name"),

		 XFIELD(CADDevice,Q_PUBLIC,portSpeed,
		       input_unsigned_type(),
		       "AD device speed, 9600 - 230400"),

		 XFIELD(CADDevice,Q_PUBLIC,state, 
			EVENT_TYPE(CADDeviceStateType, E_OUTPUT),
			"AD device state"),
		 XFIELD(CADDevice,Q_PUBLIC,channels,
			CArrayType::create(CADChannel::CADChannelType::singleton(), 16),
			"channel array"),
		 XFIELD(CADDevice, Q_PRIVATE, timeout, event_bool_type(), "Internal port reopen and init timout")

		 );
public:
    CADDevice(CExecutor* aExec, CBaseType *aType = CADDevice::CADDeviceType::singleton());
    ~CADDevice(void);
    void execute(CExecutor* aExec);
    void start(CExecutor* aExec);
    void setSampleInterval(unsigned int aChannelIndex, unsigned int aSampleInterval);

private:
    int readData(void);
    int getLine(char *aBuffer, int aMaxLength);

    void openDevice(CExecutor* aExec);
    void closeDevice(CExecutor* aExec);
    void initAD(CExecutor* aExec);
    void setupAD(CExecutor* aExec);
    void openAD(CExecutor* aExec);
    void closeAD(CExecutor* aExec);
    void processInput(CExecutor* aExec);

private:
    EventString   mPort;              // Port or file we are to read data from
    EventUnsigned mPortSpeed;         // Port speed to use bus speed
    int mWait;                  // response wait
    int mDescriptor;
    bool mSimulation;           // Debug sim mode.
    EventSigned   mRevents;     // poll events
    EventSigned   mState;             // CAN device status

    //
    // File descriptor object that ties
    // mDescriptor into the CSweeper
    // poll/scheduler system
    //
    CFileSource *mSource;

    //
    // Timeout used both for port reopen 
    // attempts and init timeout.
    //
    CTimeout *mTimeoutObj;
    EventBool mTimeout;
    char mInputBuffer[10240];
    int mInputBufferLen;
};


#endif // __AD_DEVICE_HH__

