//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005.
//

//
//  CAN232/CANUSB management protocol.
//

#ifndef __CANUSB_DEVICE_HH__
#define __CANUSB_DEVICE_HH__

#include "can_device.hh"

#define CANUSB_BUFFER_SIZE 4096
#define MAX_IQUEUE_SIZE    128
#define MAX_OQUEUE_SIZE    128

#define WAIT_NONE       0
#define WAIT_t          1
#define WAIT_T          2
#define WAIT_CLOSE      3
#define WAIT_RETRY      4    // retry init sequence

#define WAIT_INIT_DRAIN 10   // flush input (after sending C\r\r\r\r)
#define WAIT_INIT_SETUP 11   // wait for S<n>\r reply
#define WAIT_INIT_OPEN  12   // wait for O\r reply

class CCanUSBDevice: public CCanDevice {
public:
    XDERIVED_OBJECT_TYPE(CCanUSBDevice, 
			 CCanDevice, 
			 "CanUSBDevice",
			 "CanUSB driver",
			 (CCanUSBDevice_revents),
			 XFIELD(CCanUSBDevice,Q_PUBLIC,revents,
				event_signed_type(),
				"CanUSB device event input mask")
	);
public:
    CCanUSBDevice(CExecutor* aExec,
		  CBaseType *aType = CCanUSBDeviceType::singleton());
    ~CCanUSBDevice(void);
    void execute(CExecutor* aExec);
    void start(CExecutor* aExec);

private:

    void openDevice(CExecutor* aExec);
    void closeDevice(CExecutor* aExec);
    void initCAN(CExecutor* aExec);
    void setupCAN(CExecutor* aExec);
    void openCAN(CExecutor* aExec);
    void closeCAN(CExecutor* aExec);
    void processInput(CExecutor* aExec);
    void inputFrame(CExecutor* aExec,unsigned long id, int len,
		    unsigned char* data);
    void outputFrame(CExecutor* aExec,unsigned long id, int len,
		     unsigned char* data);
    struct CFrame {
	unsigned int id;
	unsigned int len;
	unsigned char data[8];
    };
    CFrame iQueue[MAX_IQUEUE_SIZE];   // circular input buffer 
    CFrame oQueue[MAX_OQUEUE_SIZE];   // circular output buffer
    int iHead,iTail;
    int oHead,oTail;


    int mWait;                  // response wait
    int mDescriptor;            // USB serial file descriptor
    EventSigned   mRevents;     // poll events

    char mReadBuffer[CANUSB_BUFFER_SIZE];
    int mCurrentBufferLength;

    //
    // File descriptor object that ties
    // mDescriptor into the CSweeper
    // poll/scheduler system
    //
    CFileSource *mSource;

    // Last time we tried to reopen a failed port || init can bus
    TimeStamp mLastAttempt;
};


#endif // __CANUSB_DEVICE_HH__

