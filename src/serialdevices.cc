/*
 *  serialdevices.cc
 *  BeebEm3
 *
 *  Created by Jon Welch on 28/08/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdio.h>
#include "serialdevices.h"
#include "6502core.h"
#include "uef.h"
#include "main.h"
#include "serial.h"
#include "beebsound.h"
#include "beebwin.h"
#include "debug.h"
#include "uefstate.h"
#include "csw.h"
#include "uservia.h"
#include "atodconv.h"
#include "serial.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <CoreFoundation/CoreFoundation.h>
#include <AvailabilityMacros.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
#include <IOKit/serial/ioss.h>
#endif
#include <IOKit/IOBSD.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef int	SOCKET;
#define SOCKET_ERROR	-1
#define INVALID_SOCKET	-1


#define TS_BUFF_SIZE	1024
#define TS_DELAY		8192			// Cycles to wait for data to be TX'd or RX'd
#define EP_DELAY		1024			// Cycles to wait for data to be TX'd or RX'd
	
extern unsigned char DCD, DCDClear, RIE;

// This bit is the Serial Port stuff
unsigned char EthernetPortEnabled;
unsigned char TouchScreenEnabled;
unsigned char ts_inbuff[TS_BUFF_SIZE];
unsigned char ts_outbuff[TS_BUFF_SIZE];
int ts_inhead, ts_intail, ts_inlen;
int ts_outhead, ts_outtail, ts_outlen;
int ts_delay;

// Stuff for ethernet port

SOCKET	mEthernetHandle = 0;
SOCKET	mListenHandle = 0;
sockaddr_in mServer;
MPTaskID mEthernetPortReadTaskID = NULL;
MPTaskID mEthernetPortStatusTaskID = NULL;
MPTaskID mListenTaskID = NULL;
WindowRef mEthernetPortWindow = NULL; 
char mIP[64] = {"127.0.0.1"};
char mPort[64] = {"23"};
bool mMode = true;
bool mStartAgain = false;

void DeactControl(OSType box)
{
#if 0//ACH - Deactivate Control
	const ControlID dbControlID = { box, 0 };
	ControlRef dbControl;
	
	GetControlByID (mEthernetPortWindow, &dbControlID, &dbControl);
    DeactivateControl(dbControl);
#endif
}

void ActControl(OSType box)
{
#if 0//ACH - ActivateControl
	const ControlID dbControlID = { box, 0 };
	ControlRef dbControl;
	
	GetControlByID (mEthernetPortWindow, &dbControlID, &dbControl);
    ActivateControl(dbControl);
#endif
}

void SetEPText(OSType box, char *text)
{
#if 0//ACH - EPText
	CFStringRef pTitle;
	
	const ControlID dbControlID = { box, 0 };
	ControlRef dbControl;
	
	GetControlByID (mEthernetPortWindow, &dbControlID, &dbControl);
	
	pTitle = CFStringCreateWithCString (kCFAllocatorDefault, text, kCFStringEncodingASCII);
	
    SetControlData(dbControl, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &pTitle);
	
	CFRelease(pTitle);
#endif
}

void GetEPText(OSType box, char *text)
{
#if 0//ACH - EPText
    ControlID kCmd = { box, 0 };
    ControlRef Cmd;
    CFStringRef cmd_text;

    *text = 0;
	
	GetControlByID(mEthernetPortWindow, &kCmd, &Cmd);
    GetControlData(Cmd, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &cmd_text, NULL);
	CFStringGetCString (cmd_text, text, 32, kCFStringEncodingASCII);
	
	CFRelease (cmd_text);
#endif
}

int GetEPValue(OSType box)

{
	int ret=0;
    ControlID dbControlID;
//    ControlRef dbControl;

	dbControlID.signature = box;
	dbControlID.id = 0;
#if 0//ACH - EPvalue
	GetControlByID (mEthernetPortWindow, &dbControlID, &dbControl);
	ret = GetControlValue(dbControl);
#endif
    return ret;
}

void SetEPValue(OSType box, int State)
{
	ControlID dbControlID;
//	ControlRef dbControl;
	
	dbControlID.signature = box;
	dbControlID.id = 0;
#if 0//ACH - EP Value
	GetControlByID (mEthernetPortWindow, &dbControlID, &dbControl);
	SetControlValue(dbControl, State);
#endif
}

#if 0//ACH - ethernet commandhandler

//*******************************************************************

OSStatus EthernetPortWindowCommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    HICommand command; 
    OSStatus err = noErr;
	int val;
	
    err = GetEventParameter(event, kEventParamDirectObject,
							typeHICommand, NULL, sizeof(HICommand), NULL, &command);
    require_noerr (err, CantGetParameter);
	
	err = noErr;
	
	switch (command.commandID)
	{
		case 'ok  ':
			
			GetEPText('cepi', mIP);
			GetEPText('cepp', mPort);

			mMode = GetEPValue('cepc');
			
            fprintf(stderr, "Selected Ethernet Port %d, '%s', %d\n", mMode, mIP, atoi(mPort));
			
			if (mEthernetHandle > 0)
			{
				EthernetPortClose();
			}
			
			EthernetPortOpen();
			
//			fprintf(stderr, "mEthernetHandle = %d\n", mEthernetHandle);
			
			if (mEthernetHandle > 0)
			{
				ts_inhead = ts_intail = ts_inlen = 0;
				ts_outhead = ts_outtail = ts_outlen = 0;
				ts_delay = 0;

				MPCreateTask(MyEthernetPortReadThread, nil, 0, nil, nil, nil, 0, &mEthernetPortReadTaskID);
				MPCreateTask(MyEthernetPortStatusThread, nil, 0, nil, nil, nil, 0, &mEthernetPortStatusTaskID);
			}
			
			EthernetPortCloseDialog();
			break;
			
		case 'cncl':
			EthernetPortCloseDialog();
			EthernetPortEnabled = false;
			mainWin->SetMenuCommandIDCheck('rs42', false);
			mainWin->SetMenuCommandIDCheck('sdep', false);
			break;

		case 'ceps':

			val = GetEPValue('ceps');
//            fprintf(stderr, "Server - val = %d\n", val);
			SetEPValue('cepc', 0);
			DeactControl('cepi');
			
			break;

		case 'cepc':

			val = GetEPValue('cepc');
//            fprintf(stderr, "Client - val = %d\n", val);
			SetEPValue('ceps', 0);
			ActControl('cepi');

			break;

		case 'cepi':
			break;

		case 'cepp':
			break;

		default:
			err = eventNotHandledErr;
			break;
	}
	
CantGetParameter:
	return err;
}

static OSStatus EthernetPortWindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    OSStatus err = noErr;
	
	switch (GetEventKind(event)) 
	{
			
        case kEventWindowClosed: 
			mEthernetPortWindow = NULL;
            break;
			
		default:
            err = eventNotHandledErr;
            break;
    }
    
	return err;
}

void EthernetPortOpenDialog()
{
	IBNibRef 		nibRef;
	EventTypeSpec EthernetPortcommands[] = {
		{ kEventClassCommand, kEventCommandProcess }
	};
	
	EventTypeSpec EthernetPortevents[] = {
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassKeyboard, kEventRawKeyDown}
	};
	
	if (mEthernetPortWindow == NULL)
	{
		// Create a Nib reference passing the name of the nib file (without the .nib extension)
		// CreateNibReference only searches into the application bundle.
		CreateNibReference(CFSTR("main"), &nibRef);
		CreateWindowFromNib(nibRef, CFSTR("Window5"), &mEthernetPortWindow);
		DisposeNibReference(nibRef);
		ShowWindow(mEthernetPortWindow);
		
		InstallWindowEventHandler(mEthernetPortWindow, 
								  NewEventHandlerUPP (EthernetPortWindowCommandHandler), 
								  GetEventTypeCount(EthernetPortcommands), EthernetPortcommands, 
								  mEthernetPortWindow, NULL);
		
		InstallWindowEventHandler (mEthernetPortWindow, 
								   NewEventHandlerUPP (EthernetPortWindowEventHandler), 
								   GetEventTypeCount(EthernetPortevents), EthernetPortevents, 
								   mEthernetPortWindow, NULL);
		
		SetEPValue('ceps', !mMode);
		SetEPValue('cepc', mMode);
		SetEPText('cepi', mIP);
		SetEPText('cepp', mPort);

		if (mMode)
		{
			ActControl('cepi');
		}
		else
		{
			DeactControl('cepi');
		}
		
	}
	
}

void EthernetPortCloseDialog()
{
	if (mEthernetPortWindow)
	{
		HideWindow(mEthernetPortWindow);
		DisposeWindow(mEthernetPortWindow);
	}
	mEthernetPortWindow = NULL;
}
#endif

void LowerDCD(void)

{
	WriteLog("Lower DCD\n");
	
	UserVIAWrite(0x0c, 0x00);
	UserVIAState.ifr |= 0x10;
	UserVIAWrite(0x0d, 0x00);		// Just to force a call to UpdateIFRTopBit
}

void RaiseDCD(void)

{
	WriteLog("Raise DCD\n");
	UserVIAWrite(0x0c, 0x00);
}

#if 0//ACH - ethernet status thread

OSStatus MyEthernetPortStatusThread(void *parameter)
{
	int dcd, odcd;

	dcd = 0;
	odcd = 0;
	
// Put bits in here for DCD when got active IP connection
	
	while (1)
	{
		
		if (mEthernetHandle > 0)
		{
			dcd = 1;
		}
		else
		{
			dcd = 0;
		}
		
		if ( (dcd == 1) && (odcd ==0) )
		{
			RaiseDCD();
		}

//		if ( (dcd == 0) && (odcd == 1) )
//		{
//			LowerDCD();
//		}

		
		if (dcd != odcd)
			odcd = dcd;
		
		usleep(1000 * 50);		// sleep for 50 milli seconds
		
	}
	
	//	fprintf(stderr, "Exited MySerialStatusThread\n");
	
	return noErr;
}

OSStatus MyEthernetPortReadThread(void *parameter)
{

fd_set	fds;
timeval tv;
int i, j;
char buff[256];
int space;
	
	while (1)
	{
		
		if (mEthernetHandle > 0)
		{
			
			space = TS_BUFF_SIZE - ts_inlen - 128;

			if (space > 0)
			{
				
				FD_ZERO(&fds);
				tv.tv_sec = 0;
				tv.tv_usec = 0;
			
				FD_SET(mEthernetHandle, &fds);
			
				i = select(32, &fds, NULL, NULL, &tv);		// Read
				if (i > 0)
				{

					i = recv(mEthernetHandle, buff, 256, 0);
					if (i > 0)
					{
					
//						WriteLog("Read %d bytes\n%s\n", i, buff);
					
						for (j = 0; j < i; j++)
						{
							EthernetPortStore(buff[j]);
						}
					}
					else
					{
//						WriteLog("Read error %d\n", i);

						mStartAgain = true;
						WriteLog("Remote session disconnected - waiting for a new connection\n");
						mEthernetPortReadTaskID = NULL;
						return noErr;
						
					}
				}
				else
				{
//					WriteLog("Nothing to read %d\n", i);
				}
			}
		}
		
		usleep(1000 * 50);		// sleep for 50 msec
	}
	
	return noErr;
}

OSStatus MyListenThread(void *parameter)
{
	int i;
	
top: ;
	
	mListenHandle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (mListenHandle <= 0)
	{
		WriteLog("Error getting socket - %08x\n", mListenHandle);
		usleep(1000 * 500);	// Sleep for 0.5 secs
		goto top;
	}
	
	mServer.sin_family = AF_INET;
	mServer.sin_port = htons(atoi(mPort));
	mServer.sin_addr.s_addr = INADDR_ANY;
	
	i = bind(mListenHandle, (sockaddr *) &mServer, sizeof(sockaddr_in));
	
	if (i < 0)
	{
		WriteLog("Error Opening Connection on bind - error %08x\n", i);
		close(mListenHandle);
		usleep(1000 * 500);	// Sleep for 0.5 secs
		goto top;
	}
	
	i = listen(mListenHandle, SOMAXCONN);
	
	if (i < 0)
	{
		WriteLog("Error Opening Connection on listen - %08x\n", i);
		close(mListenHandle);
		usleep(1000 * 500);	// Sleep for 0.5 secs
		goto top;
	}

	i = sizeof(sockaddr_in);
	
	mEthernetHandle = accept(mListenHandle, (sockaddr *) &mServer, (socklen_t *) &i);
	
	if (mEthernetHandle < 0)
	{
		WriteLog("Error Accepting Connection - %08x\n", mEthernetHandle);
		close(mListenHandle);
		usleep(1000 * 500);	// Sleep for 0.5 secs
		goto top;
	}
	
	ts_inhead = ts_intail = ts_inlen = 0;
	ts_outhead = ts_outtail = ts_outlen = 0;
	ts_delay = 0;
	
	MPCreateTask(MyEthernetPortReadThread, nil, 0, nil, nil, nil, 0, &mEthernetPortReadTaskID);
	MPCreateTask(MyEthernetPortStatusThread, nil, 0, nil, nil, nil, 0, &mEthernetPortStatusTaskID);

	WriteLog("Incoming Connection\n");

	mListenTaskID = NULL;
	return noErr;
}
#endif

void EthernetPortOpen(void)
{
#if 0//ACH - ethernetopen
	int i;
	
	if (mEthernetHandle > 0)
	{
		EthernetPortClose();
	}
	
	if (mMode == FALSE)		// Server
	{
		WriteLog("Waiting for a connection\n");
		MPCreateTask(MyListenThread, nil, 0, nil, nil, nil, 0, &mListenTaskID);
	}
	else					// Client
	{
		mEthernetHandle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		mServer.sin_family = AF_INET;
		mServer.sin_port = htons(atoi(mPort));
		mServer.sin_addr.s_addr = inet_addr(mIP);
		i = connect(mEthernetHandle, (sockaddr *) &mServer, sizeof(sockaddr_in));

		if (i < 0)
		{
			WriteLog("Error Opening Connection\n");
			EthernetPortClose();
		}
		else
		{
			WriteLog("Connection Opened\n");
		}
	}
#endif

}

bool EthernetPortPoll(void)
{

fd_set	fds;
timeval tv;
int i;
	
	if (ts_delay > 0)
	{
		ts_delay--;
		return false;
	}
	
	if (mStartAgain == true)
	{
		WriteLog("Resetting Comms\n");
		mStartAgain = false;
		EthernetPortClose();
		EthernetPortOpen();
		return false;
	}
	
	if ( (ts_outlen > 0) && (mEthernetHandle > 0) )
	{
		char buff[256];
		long bufflen = 0;

		while (ts_outlen)
		{
			buff[bufflen++] = EthernetPortGet();
		}
		
		FD_ZERO(&fds);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		
		FD_SET(mEthernetHandle, &fds);
		
		i = select(32, NULL, &fds, NULL, &tv);		// Write
		if (i <= 0)
		{
			WriteLog("Select Error %i\n", i);
		}
		else
		{
			i = (unsigned int)send(mEthernetHandle, buff, bufflen, 0);

			if (i < bufflen)
			{
				WriteLog("Send Error %i\n", i);
				mStartAgain = true;
			}
			
//			WriteLog("Written %d bytes\n%s\n", bufflen, buff);
		}
	}
	
	if (ts_inlen > 0)		// Process data waiting to be received by BeebEm straight away
	{
		return true;
	}

	return false;
}

void EthernetPortWrite(unsigned char data)
{
	
//	WriteLog("EthernetPortWrite 0x%02x\n", data);
	// Data to remote end of link
	
	if (ts_outlen != TS_BUFF_SIZE)
	{
		ts_outbuff[ts_outtail] = data;
		ts_outtail = (ts_outtail + 1) % TS_BUFF_SIZE;
		ts_outlen++;
		ts_delay = EP_DELAY;
	}
	else
	{
		WriteLog("EthernetPortWrite output buffer full\n");
	}
}

unsigned char EthernetPortGet(void)
{
	unsigned char data;
	
	data = 0;
	
	if (ts_outlen > 0)
	{
		data = ts_outbuff[ts_outhead];
		ts_outhead = (ts_outhead + 1) % TS_BUFF_SIZE;
		ts_outlen--;
	}
	
	return data;
}

unsigned char EthernetPortRead(void)
{
	unsigned char data;
	
	data = 0;
	if (ts_inlen > 0)
	{
		data = ts_inbuff[ts_inhead];
		ts_inhead = (ts_inhead + 1) % TS_BUFF_SIZE;
		ts_inlen--;
		ts_delay = EP_DELAY;
	}
	else
	{
		WriteLog("EthernetPortRead input buffer empty\n");
	}
	
//	WriteLog("EthernetPortRead 0x%02x\n", data);
	
	return data;
}

void EthernetPortStore(unsigned char data)
{
	if (ts_inlen != TS_BUFF_SIZE)
	{
		ts_inbuff[ts_intail] = data;
		ts_intail = (ts_intail + 1) % TS_BUFF_SIZE;
		ts_inlen++;
	}
	else
	{
		WriteLog("EthernetPortStore output buffer full\n");
	}
}

void EthernetPortClose(void)
{
#if 0 //ACH - ethernetclose

	if (mEthernetHandle > 0)
	{
		LowerDCD();
		close(mEthernetHandle);
	}

	if (mListenHandle > 0)
	{
		close(mListenHandle);
	}
	
	mEthernetHandle = 0;
	mListenHandle = 0;

	if (mEthernetPortReadTaskID != NULL)
	{
		MPTerminateTask(mEthernetPortReadTaskID, 0);
	}

	if (mEthernetPortStatusTaskID != NULL)
	{
		MPTerminateTask(mEthernetPortStatusTaskID, 0);
	}
	
	if (mListenTaskID != NULL)
	{
		MPTerminateTask(mListenTaskID, 0);
	}

	mListenHandle = NULL;
	mEthernetPortStatusTaskID = NULL;
	mEthernetPortReadTaskID = NULL;
	
#endif
	WriteLog("EthernetPortClose\n");
}

void TouchScreenOpen(void)
{
	ts_inhead = ts_intail = ts_inlen = 0;
	ts_outhead = ts_outtail = ts_outlen = 0;
	ts_delay = 0;
}

bool TouchScreenPoll(void)
{
unsigned char data;
static int mode = 0;

	if (ts_delay > 0)
	{
		ts_delay--;
		return false;
	}
	
	if (ts_outlen > 0)		// Process data waiting to be received by BeebEm straight away
	{
		return true;
	}

	if (ts_inlen > 0)
	{
		data = ts_inbuff[ts_inhead];
		ts_inhead = (ts_inhead + 1) % TS_BUFF_SIZE;
		ts_inlen--;
		
		switch (data)
		{
			case 'M' :
				mode = 0;
				break;

			case '0' :
			case '1' :
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
			case '9' :
				mode = mode * 10 + data - '0';
				break;

			case '.' :

/*
 * Mode 1 seems to be polled, sends a '?' and we reply with data
 * Mode 129 seems to be send current values all time
 */
				
				WriteLog("Setting touch screen mode to %d\n", mode);
				break;

			case '?' :

				if (mode == 1)		// polled mode
				{
					TouchScreenReadScreen(false);
				}
				break;
		}
	}
	
	if (mode == 129)		// real time mode
	{
		TouchScreenReadScreen(true);
	}
		
	if (mode == 130)		// area mode - seems to be pressing with two fingers which we can't really do ??
	{
		TouchScreenReadScreen(true);
	}
	
	
	return false;
}

void TouchScreenWrite(unsigned char data)
{

//	WriteLog("TouchScreenWrite 0x%02x\n", data);
	
	if (ts_inlen != TS_BUFF_SIZE)
	{
		ts_inbuff[ts_intail] = data;
		ts_intail = (ts_intail + 1) % TS_BUFF_SIZE;
		ts_inlen++;
		ts_delay = TS_DELAY;
	}
	else
	{
		WriteLog("TouchScreenWrite input buffer full\n");
	}
}


unsigned char TouchScreenRead(void)
{
unsigned char data;

	data = 0;
	if (ts_outlen > 0)
	{
		data = ts_outbuff[ts_outhead];
		ts_outhead = (ts_outhead + 1) % TS_BUFF_SIZE;
		ts_outlen--;
		ts_delay = TS_DELAY;
	}
	else
	{
		WriteLog("TouchScreenRead output buffer empty\n");
	}
	
//	WriteLog("TouchScreenRead 0x%02x\n", data);

	return data;
}

void TouchScreenClose(void)
{
}

void TouchScreenStore(unsigned char data)
{
	if (ts_outlen != TS_BUFF_SIZE)
	{
		ts_outbuff[ts_outtail] = data;
		ts_outtail = (ts_outtail + 1) % TS_BUFF_SIZE;
		ts_outlen++;
	}
	else
	{
		WriteLog("TouchScreenStore output buffer full\n");
	}
}

void TouchScreenReadScreen(bool check)
{
int x, y;
static int last_x = -1, last_y = -1, last_m = -1;

	x = (65535 - JoystickX) / (65536 / 120) + 1;
	y = JoystickY / (65536 / 90) + 1;

	if ( (last_x != x) || (last_y != y) || (last_m != AMXButtons) || (check == false))
	{

//		WriteLog("JoystickX = %d, JoystickY = %d, last_x = %d, last_y = %d\n", JoystickX, JoystickY, last_x, last_y);
		
		if (AMXButtons & AMX_LEFT_BUTTON)
		{
			TouchScreenStore( 64 + ((x & 0xf0) >> 4));
			TouchScreenStore( 64 + (x & 0x0f));
			TouchScreenStore( 64 + ((y & 0xf0) >> 4));
			TouchScreenStore( 64 + (y & 0x0f));
			TouchScreenStore('.');
//			WriteLog("Sending X = %d, Y = %d\n", x, y);
		} else {
			TouchScreenStore(64 + 0x0f);
			TouchScreenStore(64 + 0x0f);
			TouchScreenStore(64 + 0x0f);
			TouchScreenStore(64 + 0x0f);
			TouchScreenStore('.');
//			WriteLog("Screen not touched\n");
		}
		last_x = x;
		last_y = y;
		last_m = AMXButtons;
	}
}

#if 0 //ACH - serialportcommandhandler

// Stuff for proper serial port

WindowRef mSerialPortWindow = NULL; 
ControlRef mPopup = NULL;
char mBSDName[20][100];
char mPortName[20][100];
struct termios gOriginalTTYAttrs;
int mSerialHandle = -1;
MPTaskID mSerialReadTaskID = NULL;
MPTaskID mSerialStatusTaskID = NULL;
unsigned char RXBuff[4096];
int RXHead = 0;
int RXTail = 0;
int RXLen = 0;

//*******************************************************************

OSStatus SerialPortWindowCommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    HICommand command; 
    OSStatus err = noErr;
	int a;
	
    err = GetEventParameter(event, kEventParamDirectObject,
							typeHICommand, NULL, sizeof(HICommand), NULL, &command);
    require_noerr (err, CantGetParameter);
	
	err = noErr;

	switch (command.commandID)
	{
		case 'ok  ':

			a = GetControl32BitValue(mPopup);

            fprintf(stderr, "Selected Serial Port %d, '%s', '%s'\n", a, mBSDName[a - 1], mPortName[a - 1]);

			if (mSerialHandle != -1)
			{
				CloseSerialPort(mSerialHandle);
			}
				
			mSerialHandle = OpenSerialPort(mBSDName[a - 1]);

			fprintf(stderr, "mSerialHandle = %d\n", mSerialHandle);
			
			if (mSerialHandle != -1)
			{
				RXTail = 0;
				RXHead = 0;
				RXLen = 0;
			
				MPCreateTask(MySerialReadThread, nil, 0, nil, nil, nil, 0, &mSerialReadTaskID);
				MPCreateTask(MySerialStatusThread, nil, 0, nil, nil, nil, 0, &mSerialStatusTaskID);
			}
				
			SerialPortCloseDialog();
			break;
				
		case 'cncl':
			SerialPortCloseDialog();
			SerialPortEnabled = false;
			mainWin->SetMenuCommandIDCheck('rs42', false);
			mainWin->SetMenuCommandIDCheck('sdsp', false);
			break;

		default:
			err = eventNotHandledErr;
			break;
	}

CantGetParameter:
		return err;
}

static OSStatus SerialPortWindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    OSStatus err = noErr;

	switch (GetEventKind(event)) 
	{

        case kEventWindowClosed: 
			mSerialPortWindow = NULL;
            break;
        
		default:
            err = eventNotHandledErr;
            break;
    }
    
	return err;
}

void SerialPortOpenDialog()
{
	IBNibRef 		nibRef;
	EventTypeSpec SerialPortcommands[] = {
	{ kEventClassCommand, kEventCommandProcess }
	};
		
	EventTypeSpec SerialPortevents[] = {
	{ kEventClassWindow, kEventWindowClosed },
	{ kEventClassKeyboard, kEventRawKeyDown}
	};

	if (mSerialPortWindow == NULL)
	{
		// Create a Nib reference passing the name of the nib file (without the .nib extension)
		// CreateNibReference only searches into the application bundle.
		CreateNibReference(CFSTR("main"), &nibRef);
		CreateWindowFromNib(nibRef, CFSTR("Window4"), &mSerialPortWindow);
		DisposeNibReference(nibRef);
		ShowWindow(mSerialPortWindow);
		
		InstallWindowEventHandler(mSerialPortWindow, 
							  NewEventHandlerUPP (SerialPortWindowCommandHandler), 
							  GetEventTypeCount(SerialPortcommands), SerialPortcommands, 
							  mSerialPortWindow, NULL);
		
		InstallWindowEventHandler (mSerialPortWindow, 
								   NewEventHandlerUPP (SerialPortWindowEventHandler), 
								   GetEventTypeCount(SerialPortevents), SerialPortevents, 
								   mSerialPortWindow, NULL);
		

		Rect r;
		io_iterator_t	serialPortIterator;
		int i = 0;
		kern_return_t			kernResult; 
		CFMutableDictionaryRef	classesToMatch;
		
		r.top = 20;
		r.left = 20;
		r.right = r.left + 184;
		r.bottom = r.top + 20;
		
		CreatePopupButtonControl (mSerialPortWindow, &r, NULL, 
								  -12345,	// DON'T GET MENU FROM RESOURCE mMenuID,!!!
								  FALSE,	// variableWidth, 
								  0,		// titleWidth, 
								  0,		// titleJustification, 
								  0,		// titleStyle, 
								  &mPopup);
		
		MenuRef menuRef;
		CreateNewMenu(1, 0, &menuRef);
		
		classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
		if (classesToMatch == NULL)
		{
			fprintf(stderr, "IOServiceMatching returned a NULL dictionary.\n");
		}
		else {

			CFDictionarySetValue(classesToMatch,
								 CFSTR(kIOSerialBSDTypeKey),
								 CFSTR(kIOSerialBSDAllTypes));
			
			kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &serialPortIterator);    
			if (KERN_SUCCESS != kernResult)
			{
				fprintf(stderr, "IOServiceGetMatchingServices returned %d\n", kernResult);
			}
			else
			{
				io_object_t		modemService;
					
				while (modemService = IOIteratorNext(serialPortIterator))
				{
					CFTypeRef	bsdPathAsCFString;
						
/*
 kIOTTYDeviceKey = <Raw Unique Device Name>; - usbserial
 kIOTTYBaseNameKey = <Raw Unique Device Name>; - usbserial
 kIOTTYSuffixKey = <Raw Unique Device Name>; - blank
 kIOCalloutDeviceKey = <Callout Device Name>; - /dev/cu.usbserial
 kIODialinDeviceKey = <Dialin Device Name>; - /dev/tty.usbserial
 */
					
					bsdPathAsCFString = IORegistryEntryCreateCFProperty(modemService,
																		CFSTR(kIOTTYDeviceKey),
																		kCFAllocatorDefault,
																		0);

					if (bsdPathAsCFString)
					{

						AppendMenuItemTextWithCFString (menuRef, (CFStringRef) bsdPathAsCFString, 0, 0, 0);

						CFStringGetCString((CFStringRef) bsdPathAsCFString, mPortName[i], 100, kCFStringEncodingUTF8);

						CFRelease(bsdPathAsCFString);
						
					}

					bsdPathAsCFString = IORegistryEntryCreateCFProperty(modemService,
																		CFSTR(kIOCalloutDeviceKey),
																		kCFAllocatorDefault,
																		0);
					
					if (bsdPathAsCFString)
					{
						CFStringGetCString((CFStringRef) bsdPathAsCFString, mBSDName[i], 100, kCFStringEncodingUTF8);
						CFRelease(bsdPathAsCFString);
						i++;
					}
					
					(void) IOObjectRelease(modemService);
				}

			}
		
			IOObjectRelease(serialPortIterator);	// Release the iterator.
		
		}
		
		SetControlData(mPopup, 0, kControlPopupButtonMenuRefTag, sizeof(menuRef), &menuRef);
		
		SetControl32BitMaximum (mPopup, i);				// Set number of menu items

		SetControl32BitValue (mPopup, 1);				// Set default menu item

	}

}

void SerialPortCloseDialog()
{
	if (mSerialPortWindow)
	{
		HideWindow(mSerialPortWindow);
		DisposeWindow(mSerialPortWindow);
	}
	mSerialPortWindow = NULL;
}

// Given the path to a serial device, open the device and configure it.
// Return the file descriptor associated with the device.
int OpenSerialPort(const char *bsdPath)
{
    int				fileDescriptor = -1;
    int				handshake;
    struct termios	options;
	int				flags;
	
    // Open the serial port read/write, with no controlling terminal, and don't wait for a connection.
    // The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking.
    // See open(2) ("man 2 open") for details.
    
    fileDescriptor = open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
    if (fileDescriptor == -1)
    {
        fprintf(stderr, "Error opening serial port %s - %s (%d)\n",
               bsdPath, strerror(errno), errno);
        goto error;
    }
	
    // Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
    // unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
    // processes.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
    
    if (ioctl(fileDescriptor, TIOCEXCL) == -1)
    {
        fprintf(stderr, "Error setting TIOCEXCL on %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
    // See fcntl(2) ("man 2 fcntl") for details.
    
    flags = fcntl(fileDescriptor, F_GETFL, 0);
		
	if (fcntl(fileDescriptor, F_SETFL, flags | O_ASYNC) == -1)
    {
        fprintf(stderr, "Error setting O_ASYNC %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // Get the current options and save them so we can restore the default settings later.
    if (tcgetattr(fileDescriptor, &gOriginalTTYAttrs) == -1)
    {
        fprintf(stderr, "Error getting tty attributes %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
        goto error;
    }
	
    // The serial port attributes such as timeouts and baud rate are set by modifying the termios
    // structure and then calling tcsetattr() to cause the changes to take effect. Note that the
    // changes will not become effective without the tcsetattr() call.
    // See tcsetattr(4) ("man 4 tcsetattr") for details.
    
    options = gOriginalTTYAttrs;
    
    // Print the current input and output baud rates.
    // See tcsetattr(4) ("man 4 tcsetattr") for details.
    
    fprintf(stderr, "Current input baud rate is %d\n", (int) cfgetispeed(&options));
    fprintf(stderr, "Current output baud rate is %d\n", (int) cfgetospeed(&options));
    
    // Set raw input (non-canonical) mode, with reads blocking until either a single character 
    // has been received or a one second timeout expires.
    // See tcsetattr(4) ("man 4 tcsetattr") and termios(4) ("man 4 termios") for details.
    
    cfmakeraw(&options);
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 20;
	
    // The baud rate, word length, and handshake options can be set as follows:
    
    cfsetspeed(&options, B19200);	   // Set 19200 baud    

	fprintf(stderr, "Options flag = %08x\n", (unsigned int) options.c_cflag);

	options.c_cflag |= CS8;

	// 8N1
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    // no flow control
    options.c_cflag &= ~CRTSCTS;
	
    options.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
	
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    options.c_oflag &= ~OPOST; // make raw
		
//	options.c_cflag |= CCTS_OFLOW;   // CTS flow control of output
//	options.c_cflag &= ~CRTS_IFLOW;   // RTS flow control of input
	
    // Print the new input and output baud rates. Note that the IOSSIOSPEED ioctl interacts with the serial driver 
	// directly bypassing the termios struct. This means that the following two calls will not be able to read
	// the current baud rate if the IOSSIOSPEED ioctl was used but will instead return the speed set by the last call
	// to cfsetspeed.
    
    fprintf(stderr, "Input baud rate changed to %d\n", (int) cfgetispeed(&options));
    fprintf(stderr, "Output baud rate changed to %d\n", (int) cfgetospeed(&options));
    
    // Cause the new options to take effect immediately.
    if (tcsetattr(fileDescriptor, TCSANOW, &options) == -1)
    {
        fprintf(stderr, "Error setting tty attributes %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
        goto error;
    }
	
    // To set the modem handshake lines, use the following ioctls.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
    
    if (ioctl(fileDescriptor, TIOCSDTR) == -1) // Assert Data Terminal Ready (DTR)
    {
        fprintf(stderr, "Error asserting DTR %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
    }
    
    if (ioctl(fileDescriptor, TIOCCDTR) == -1) // Clear Data Terminal Ready (DTR)
    {
        fprintf(stderr, "Error clearing DTR %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
    }
    
    handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
    if (ioctl(fileDescriptor, TIOCMSET, &handshake) == -1)
		// Set the modem lines depending on the bits set in handshake
    {
        fprintf(stderr, "Error setting handshake lines %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
    }
    
    // To read the state of the modem lines, use the following ioctl.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
    
    if (ioctl(fileDescriptor, TIOCMGET, &handshake) == -1)
		// Store the state of the modem lines in handshake
    {
        fprintf(stderr, "Error getting handshake lines %s - %s (%d)\n",
			   bsdPath, strerror(errno), errno);
    }
    
    fprintf(stderr, "Handshake lines currently set to %d\n", handshake);
	
    // Success
    return fileDescriptor;
    
    // Failure path
error:
		if (fileDescriptor != -1)
		{
			close(fileDescriptor);
		}
    
    return -1;
}

// Given the file descriptor for a serial device, close that device.
void CloseSerialPort(int fileDescriptor)
{
    // Block until all written output has been sent from the device.
    // Note that this call is simply passed on to the serial device driver. 
	// See tcsendbreak(3) ("man 3 tcsendbreak") for details.
    if (tcdrain(fileDescriptor) == -1)
    {
        fprintf(stderr, "Error waiting for drain - %s (%d)\n",
			   strerror(errno), errno);
    }
    
    // Traditionally it is good practice to reset a serial port back to
    // the state in which you found it. This is why the original termios struct
    // was saved.
    if (tcsetattr(fileDescriptor, TCSANOW, &gOriginalTTYAttrs) == -1)
    {
        fprintf(stderr, "Error resetting tty attributes - %s (%d)\n",
			   strerror(errno), errno);
    }
	
	MPTerminateTask(mSerialReadTaskID, 0);
	MPTerminateTask(mSerialStatusTaskID, 0);
    
	close(fileDescriptor);

	mSerialHandle = -1;
}

OSStatus MySerialStatusThread(void *parameter)
{
int lasthandshake = -1;
int	handshake;
	
//	fprintf(stderr, "In MySerialStatusThread - mSerialHandle = %d\n", mSerialHandle);


	while (1)
	{

		if (ioctl(mSerialHandle, TIOCMGET, &handshake) != -1)
		{
			if (handshake != lasthandshake)
			{

//				fprintf(stderr, "In MySerialStatusThread - Handshake now %08x\n", handshake);

				if (handshake & TIOCM_CTS) ResetACIAStatus(3); else SetACIAStatus(3);		// Invert for CTS bit

				lasthandshake = handshake;
			}
		}

		usleep(100);		// sleep for 0.1 milli second
		
	}
	
//	fprintf(stderr, "Exited MySerialStatusThread\n");

	return noErr;
}

OSStatus MySerialReadThread(void *parameter)
{
unsigned char buff[1024];
int a, b;
int bytes;
	
//	fprintf(stderr, "In MySerialReadThread - mSerialHandle = %d\n", mSerialHandle);
	
	while (1)
	{
		
		ioctl(mSerialHandle, FIONREAD, &bytes);
		if (bytes > 0)
		{

			b = (unsigned int)read(mSerialHandle, buff, bytes);
			if (b > 0)
			{
				for (a = 0; a < b; a++)
				{
					RXBuff[RXTail] = buff[a];
					RXTail = (RXTail + 1) % 4096;
				}
				RXLen += b;
			}
		}
		
		usleep(50);		// sleep for 50 usec
	}
	
//	fprintf(stderr, "Exited MySerialReadThread\n");
	
	return noErr;
}


void SetSerialPortFormat(int Data_Bits, int Stop_Bits, int Parity, int RTS)
{
struct termios	options;
static int last_Data_Bits = -1;
static int last_Stop_Bits = -1;
static int last_Parity = -1;
static int last_RTS = -1;

	
//	fprintf(stderr, "SetSerialPortFormat - mSerialHandle = %d, Data_Bits = %d, Stop_Bits = %d, Parity = %d, RTS = %d\n", 
//			mSerialHandle, Data_Bits, Stop_Bits, Parity, RTS);

	if ( (Data_Bits != last_Data_Bits) || (Stop_Bits != last_Stop_Bits) || (Parity != last_Parity) || (RTS != last_RTS) )

	{
		
		if (tcgetattr(mSerialHandle, &options) != -1)
		{
			if (Data_Bits == 7) {
				options.c_cflag &= ~CSIZE;
				options.c_cflag |= CS7;
			}
			if (Data_Bits == 8) {
				options.c_cflag &= ~CSIZE;
				options.c_cflag |= CS8;
			}
		
			if (Stop_Bits == 1) {
				options.c_cflag &= ~CSTOPB;		// Turn off two stop bits
			} else {
				options.c_cflag |= CSTOPB;		// Set two stop bits
			}
		
			if (Parity == 0) {					// No parity
				options.c_cflag &= ~PARENB;
			} else if (Parity == 1) {			// Odd Parity
				options.c_cflag |= PARENB;		// Enable parity
				options.c_cflag |= PARODD;		// Set odd parity
			} else {							// Even Parity
				options.c_cflag |= PARENB;		// Enable parity
				options.c_cflag &= ~PARODD;		// Set even parity
			}
		
//		if (RTS == 1) {
//			options.c_cflag &= ~CRTSCTS;	// Turn off flow control
//		} else {
//			options.c_cflag |= CRTSCTS;		// Turn on flow control
//		}
//	
//		options.c_cflag |= CRTSCTS;		// Turn on flow control
//
//		options.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
//		options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
		
			if (tcsetattr(mSerialHandle, TCSANOW, &options) == -1)
			{
				fprintf(stderr, "SetSerialPortFormat - Error setting attributes\n");
			} else {
				last_Data_Bits = Data_Bits;
				last_Stop_Bits = Stop_Bits;
				last_Parity = Parity;
				last_RTS = RTS;
			}
		}
		else
		{
			fprintf(stderr, "SetSerialPortFormat - Error reading attributes\n");
		}
	}

	RXTail = 0;
	RXHead = 0;
	RXLen = 0;
}

void SetSerialPortBaud(int Tx_Rate, int Rx_Rate)
{

struct termios	options;

	fprintf(stderr, "SetSerialPortBaud - mSerialHandle = %d, Tx_Rate = %d, Rx_Rate = %d\n", mSerialHandle, Tx_Rate, Rx_Rate);

//	if (Tx_Rate > Rx_Rate) Rx_Rate = Tx_Rate;
//	if (Rx_Rate > Tx_Rate) Tx_Rate = Rx_Rate;
	
    if (tcgetattr(mSerialHandle, &options) != -1)
    {
		cfsetispeed(&options, Rx_Rate);	   // Set RX baud    
		cfsetospeed(&options, Tx_Rate);	   // Set TX baud    
		tcsetattr(mSerialHandle, TCSANOW, &options);
	}

	RXTail = 0;
	RXHead = 0;
	RXLen = 0;
}

void SerialPortWrite(unsigned char TDR)
{

//	fprintf(stderr, "SerialPortWrite - mSerialHandle = %d, data = %d\n", mSerialHandle, TDR);

	write(mSerialHandle, &TDR, 1);
}

unsigned char SerialPortGetChar(void)

{
unsigned char RXD;
	
	RXD = RXBuff[RXHead];
	RXHead = (RXHead + 1) % 4096;
	RXLen--;

	return RXD;
}

int SerialPortIsChar(void)

{
	return RXLen;
}
#endif
