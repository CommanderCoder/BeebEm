/*
Serial/Cassette Support for BeebEm

Written by Richard Gellman - March 2001

You may not distribute this entire file separate from the whole BeebEm distribution.

You may use sections or all of this code for your own uses, provided that:

1) It is not a separate file in itself.
2) This copyright message is included
3) Acknowledgement is made to the author, and any aubsequent authors of additional code.

The code may be modified as required, and freely distributed as such authors see fit,
provided that:

1) Acknowledgement is made to the author, and any subsequent authors of additional code
2) Indication is made of modification.

Absolutely no warranties/guarantees are made by using this code. You use and/or run this code
at your own risk. The code is classed by the author as "unlikely to cause problems as far as
can be determined under normal use".

-- end of legal crap - Richard Gellman :) */

// P.S. If anybody knows how to emulate this, do tell me - 16/03/2001 - Richard Gellman

// Need to comment out uef calls and remove uef.lib from project in
// order to use VC profiling.
//#define PROFILING

#include <stdio.h>
#include "6502core.h"
#include "uef.h"
#include "main.h"
#include "serial.h"
#include "beebsound.h"
#include "beebwin.h"
#include "debug.h"
#include "uefstate.h"
#include "csw.h"
#include "serialdevices.h"

#define CASSETTE 0  // Device in 
#define RS423 1		// use defines

unsigned char Cass_Relay=0; // Cassette Relay state
unsigned char SerialChannel=CASSETTE; // Device in use

unsigned char RDR,TDR; // Receive and Transmit Data Registers
unsigned char RDSR,TDSR; // Receive and Transmit Data Shift Registers (buffers)
unsigned int Tx_Rate=1200,Rx_Rate=1200; // Recieve and Transmit baud rates.
unsigned char Clk_Divide=1; // Clock divide rate

unsigned char ACIA_Status,ACIA_Control; // 6850 ACIA Status.& Control
unsigned char SP_Control; // SERPROC Control;

unsigned char CTS,RTS,FirstReset=1;
unsigned char DCD=0,DCDI=1,ODCDI=1,DCDClear=0; // count to clear DCD bit

unsigned char Parity,Stop_Bits,Data_Bits,RIE,TIE; // Receive Intterrupt Enable
												  // and Transmit Interrupt Enable
unsigned char TxD,RxD; // Transmit and Receive destinations (data or shift register)

char UEFTapeName[256]; // Filename of current tape file
unsigned char UEFOpen=0;

unsigned int Baud_Rates[8]={19200,1200,4800,150,9600,300,2400,75};

unsigned char OldRelayState=0;
CycleCountT TapeTrigger=CycleCountTMax;
#define TAPECYCLES 357 // 2000000/5600 - 5600 is normal tape speed

int UEF_BUF=0,NEW_UEF_BUF=0;
int TapeClock=0,OldClock=0;
int TapeClockSpeed = 5600;
int UnlockTape=1;

// Tape control variables
int map_lines;
char map_desc[MAX_MAP_LINES][40];
int map_time[MAX_MAP_LINES];
bool TapeControlEnabled = false;
bool TapePlaying = true;
bool TapeRecording = false;
void TapeControlOpenFile(char *file_name);
void TapeControlUpdateCounter(int tape_time);
void TapeControlStopRecording(bool RefreshControl);

// This bit is the Serial Port stuff
unsigned char SerialPortEnabled;
unsigned char SerialPort;
unsigned char SerialPortOpen;
unsigned int SerialBuffer = 0, SerialWriteBuffer = 0;

FILE *serlog;


extern "C" void swift_SelectItem(const char* tableid, long numberOfSelections, DataBrowserItemID *listOfSelections);
extern "C" void swift_UEFNewFile(const char* filename);
extern "C" long swift_Alert(const char* line1, const char* line2, bool hasCancel);
extern "C" void swift_UpdateItem(const char* menu, long numberOfItems);

WindowRef mTCWindow = NULL; 

void SetACIAStatus(unsigned char bit) {
	ACIA_Status|=1<<bit;
}

void ResetACIAStatus(unsigned char bit) {
	ACIA_Status&=~(1<<bit);
}

void Write_ACIA_Control(unsigned char CReg) {
	unsigned char bit;
	if (DebugEnabled) {
		char info[200];
		sprintf(info, "Serial: Write ACIA control %02X", (int)CReg);
		DebugDisplayTrace(DEBUG_SERIAL, true, info);
	}

	ACIA_Control=CReg; // This is done for safe keeping
	// Master reset
	if ((CReg & 3)==3) {
		ACIA_Status&=8; ResetACIAStatus(7); SetACIAStatus(2);
		intStatus&=~(1<<serial); // Master reset clears IRQ
		if (FirstReset==1) { 
			CTS=1; SetACIAStatus(3);
			FirstReset=0; RTS=1; 
		} // RTS High on first Master reset.
		ResetACIAStatus(2); DCD=0; DCDI=0; DCDClear=0;
		SetACIAStatus(1); // Xmit data register empty
		TapeTrigger=TotalCycles+TAPECYCLES;
	}
	// Clock Divide
	if ((CReg & 3)==0) Clk_Divide=1;
	if ((CReg & 3)==1) Clk_Divide=16;
	if ((CReg & 3)==2) Clk_Divide=64;

	// Parity, Data, and Stop Bits.
	Parity=2-((CReg & 4)>>2);
	Stop_Bits=2-((CReg & 8)>>3);
	Data_Bits=7+((CReg & 16)>>4);
	if ((CReg & 28)==16) { Stop_Bits=2; Parity=0; }
	if ((CReg & 28)==20) { Stop_Bits=1; Parity=0; }
	// Transmission control
	TIE=(CReg & 32)>>5;
	RTS=(CReg & 64)>>6;
	RIE=(CReg & 128)>>7;
	bit=(CReg & 96)>>5;
	if (bit==3) { RTS=0; TIE=0; }
	// Seem to need an interrupt immediately for tape writing when TIE set
	if ( (SerialChannel == CASSETTE) && TIE && (Cass_Relay == 1) ) {
		intStatus|=1<<serial;
		SetACIAStatus(7);
	}

	// Change serial port settings
	if ((SerialChannel==RS423) && (SerialPortEnabled) && (mSerialHandle != -1) ) {
		SetSerialPortFormat(Data_Bits, Stop_Bits, Parity, RTS);
	}
}

void Write_ACIA_Tx_Data(unsigned char Data) {
	if (DebugEnabled) {
		char info[200];
		sprintf(info, "Serial: Write ACIA Tx %02X", (int)Data);
		DebugDisplayTrace(DEBUG_SERIAL, true, info);
	}

	intStatus&=~(1<<serial);
	ResetACIAStatus(7);
	
/*
 * 10/09/06
 * JW - A bug in swarm loader overwrites the rs423 output buffer counter
 * Unless we do something with the data, the loader hangs so just swallow it (see below)
 */

	if ( (SerialChannel==CASSETTE) || ((SerialChannel==RS423) && (!SerialPortEnabled)) )
    {
		ResetACIAStatus(1);
		TDR=Data;
		TxD=1;
		int baud = Tx_Rate * ((Clk_Divide==1) ? 64 : (Clk_Divide==64) ? 1 : 4);
		TapeTrigger=TotalCycles + 2000000/(baud/8) * TapeClockSpeed/5600;
	}


	if ((SerialChannel==RS423) && (SerialPortEnabled))
	{
		
		if (mSerialHandle != -1)
		{
			ResetACIAStatus(1);
			TDR = Data;
			TxD = 1;
			int baud = Tx_Rate * ((Clk_Divide==1) ? 64 : (Clk_Divide==64) ? 1 : 4);
			TapeTrigger=TotalCycles + 2000000/(baud/8) * TapeClockSpeed/5600;
//			fprintf(stderr, "Waiting %f cycles\n", 2000000/(baud/8) * TapeClockSpeed/5600.0);
		}
		else
		{
			if (ACIA_Status & 2) 
			{
				ResetACIAStatus(1);
				SerialWriteBuffer=Data;

				if (TouchScreenEnabled)
				{
					TouchScreenWrite(Data);
				}

				if (EthernetPortEnabled)
				{
					EthernetPortWrite(Data);
				}
				
				SetACIAStatus(1);
			}
		}
	}
}

void Write_SERPROC(unsigned char Data) {
	int HigherRate;
	
	if (DebugEnabled) {
		char info[200];
		sprintf(info, "Serial: Write serial ULA %02X", (int)Data);
		DebugDisplayTrace(DEBUG_SERIAL, true, info);
	}
	SP_Control=Data;
	// Slightly easier this time.
	// just the Rx and Tx baud rates, and the selectors.
	Cass_Relay=(Data & 128)>>7;
	TapeAudio.Enabled=(Cass_Relay && (TapePlaying||TapeRecording))?TRUE:FALSE;
	LEDs.Motor=(Cass_Relay==1);
	if (Cass_Relay)
		TapeTrigger=TotalCycles+TAPECYCLES;
	if (Cass_Relay!=OldRelayState) {
		OldRelayState=Cass_Relay;
		ClickRelay(Cass_Relay);
	}
	SerialChannel=(Data & 64)>>6;
	Tx_Rate=Baud_Rates[(Data & 7)];
	Rx_Rate=Baud_Rates[(Data & 56)>>3];

	// Note, the PC serial port (or at least win32) does not allow different transmit/receive rates
	// So we will use the higher of the two
	if ( (SerialChannel==RS423) && (mSerialHandle != -1) ) {
		HigherRate=Tx_Rate;
		if (Rx_Rate>Tx_Rate) HigherRate=Rx_Rate;
		SetSerialPortBaud(Tx_Rate, Rx_Rate);
	}
}

unsigned char Read_ACIA_Status(void) {
//	if (DCDI==0 && DCD!=0)
//	{
//		DCDClear++;
//		if (DCDClear > 1) {
//			DCD=0; ResetACIAStatus(2);
//			DCDClear=0;
//		}
//	}
	if (DebugEnabled) {
		char info[200];
		sprintf(info, "Serial: Read ACIA status %02X", (int)ACIA_Status);
		DebugDisplayTrace(DEBUG_SERIAL, true, info);
	}

	return(ACIA_Status);
}

void HandleData(unsigned char AData) {
	//fprintf(serlog,"%d %02X\n",RxD,AData);
	// This proc has to dump data into the serial chip's registers
	if (RxD==0) { RDR=AData; SetACIAStatus(0); } // Rx Reg full
	if (RxD==1) { RDSR=AData; SetACIAStatus(0); }
	ResetACIAStatus(5);
	if (RxD==2) { RDR=RDSR; RDSR=AData; SetACIAStatus(5); } // overrun
	if (RIE) { intStatus|=1<<serial; SetACIAStatus(7); } // interrupt on receive/overun
	if (RxD<2) RxD++; 	
}


unsigned char Read_ACIA_Rx_Data(void) {
	unsigned char TData;
//	if (DCDI==0 && DCD!=0)
//	{
//		DCDClear++;
//		if (DCDClear > 1) {
//			DCD=0; ResetACIAStatus(2);
//			DCDClear=0;
//		}
//	}
	intStatus&=~(1<<serial);
	ResetACIAStatus(7);
	TData=RDR; RDR=RDSR; RDSR=0;
	if (RxD>0) RxD--; 
	if (RxD==0) ResetACIAStatus(0);
	if ((RxD>0) && (RIE)) { intStatus|=1<<serial; SetACIAStatus(7); }
	if (Data_Bits==7) TData&=127;
	if (DebugEnabled) {
		char info[200];
		sprintf(info, "Serial: Read ACIA Rx %02X", (int)TData);
		DebugDisplayTrace(DEBUG_SERIAL, true, info);
	}
	return(TData);
}

unsigned char Read_SERPROC(void) {
	if (DebugEnabled) {
		char info[200];
		sprintf(info, "Serial: Read serial ULA %02X", (int)SP_Control);
		DebugDisplayTrace(DEBUG_SERIAL, true, info);
	}
	return(SP_Control);
}

void Serial_Poll(void)
{

static bool wait_for_tx = false;
static int delay = 0;
	if ((SerialChannel==RS423) && (SerialPortEnabled) && (TouchScreenEnabled) )
	{
		if (TouchScreenPoll() == true)
		{
			if (RxD<2)
				HandleData(TouchScreenRead());
		}
	}

	if ((SerialChannel==RS423) && (SerialPortEnabled) && (EthernetPortEnabled) )
	{

		if (EthernetPortPoll() == true)
		{
			if (RxD<2)
				HandleData(EthernetPortRead());
		}
	}
	
	
	if ((SerialChannel==RS423) && (SerialPortEnabled) && (mSerialHandle != -1) )
	{
		if (TxD > 0)
		{
			SerialPortWrite(TDR);
			TxD = 0;
			wait_for_tx = true;			// Wait a bit until set transmitter empty interrupt
			delay = 1;
		}
		
		if (delay > 0)
		{
			delay--;
			if (delay == 0)
			{
				SetACIAStatus(1);
			}
		}
		
//		if (wait_for_tx == true)
//		{
//			if (TotalCycles >= TapeTrigger)
//			{
//				SetACIAStatus(1);
//				wait_for_tx = false;
//			}
//		}

		if ( (SerialPortIsChar() > 0) && (RxD < 2) )
		{
			HandleData(SerialPortGetChar());
		}
		
	}
    
	if (SerialChannel==CASSETTE)
	{
		if (TapeRecording)
		{
			if (Cass_Relay==1 && UEFOpen && TotalCycles >= TapeTrigger)
			{
				if (TxD > 0)
				{
					// Writing data
					if (!uef_putdata(TDR|UEF_DATA, TapeClock))
					{
						char errstr[256];
						sprintf(errstr, "Error writing to UEF file:\n  %s", UEFTapeName);
//						MessageBox(GETHWND,errstr,"BeebEm",MB_ICONERROR|MB_OK);
						TapeControlStopRecording(true);
					}
					TxD=0;
					SetACIAStatus(1);
					if (TIE)
					{
						intStatus|=1<<serial;
						SetACIAStatus(7);
					}
					TapeAudio.Data=(TDR<<1)|1;
					TapeAudio.BytePos=1;
					TapeAudio.CurrentBit=0;
					TapeAudio.Signal=1;
					TapeAudio.ByteCount=3;
				}
				else
				{
					// Tone
					if (!uef_putdata(UEF_HTONE, TapeClock))
					{
						char errstr[256];
						sprintf(errstr, "Error writing to UEF file:\n  %s", UEFTapeName);
//						MessageBox(GETHWND,errstr,"BeebEm",MB_ICONERROR|MB_OK);
						TapeControlStopRecording(true);
					}
					TapeAudio.Signal=2;
					TapeAudio.BytePos=11;

				}

				TapeTrigger=TotalCycles+TAPECYCLES;
				TapeClock++;
			}
		}
		else // Playing or stopped
		{

/*
 * 10/09/06
 * JW - If trying to write data when not recording, just ignore
 */
			
			if ( (TxD > 0) && (TotalCycles >= TapeTrigger) )
			{
				
				//				WriteLog("Ignoring Writes\n");
				
				TxD=0;
				SetACIAStatus(1);
				if (TIE)
				{
					intStatus|=1<<serial;
					SetACIAStatus(7);
				}
			}
			
			
			if (Cass_Relay==1 && UEFOpen && TapeClock!=OldClock)
			{
				NEW_UEF_BUF=uef_getdata(TapeClock);
				OldClock=TapeClock;
			}

			if ((NEW_UEF_BUF!=UEF_BUF || UEFRES_TYPE(NEW_UEF_BUF)==UEF_HTONE || UEFRES_TYPE(NEW_UEF_BUF)==UEF_GAP) &&
				(Cass_Relay==1) && (UEFOpen))
			{
				if (UEFRES_TYPE(UEF_BUF) != UEFRES_TYPE(NEW_UEF_BUF))
					TapeControlUpdateCounter(TapeClock);
		
				UEF_BUF=NEW_UEF_BUF;

				// New data read in, so do something about it
				if (UEFRES_TYPE(UEF_BUF) == UEF_HTONE)
				{
					DCDI=1;
					TapeAudio.Signal=2;
					//TapeAudio.Samples=0;
					TapeAudio.BytePos=11;
				}
				if (UEFRES_TYPE(UEF_BUF) == UEF_GAP)
				{
					DCDI=1;
					TapeAudio.Signal=0;
				}
				if (UEFRES_TYPE(UEF_BUF) == UEF_DATA)
				{
					DCDI=0;
					HandleData(UEFRES_BYTE(UEF_BUF));
					TapeAudio.Data=(UEFRES_BYTE(UEF_BUF)<<1)|1;
					TapeAudio.BytePos=1;
					TapeAudio.CurrentBit=0;
					TapeAudio.Signal=1;
					TapeAudio.ByteCount=3;
				}
			}
			if ((Cass_Relay==1) && (RxD<2) && UEFOpen)
			{
				if (TotalCycles >= TapeTrigger)
				{
					if (TapePlaying)
						TapeClock++;
					TapeTrigger=TotalCycles+TAPECYCLES;
				}
			}

// CSW stuff			

			if (Cass_Relay == 1 && CSWOpen && TapeClock != OldClock)
			{
				int last_state = csw_state;
				
				CSW_BUF = csw_poll(TapeClock);
				OldClock = TapeClock;
			
				if (last_state != csw_state)
					TapeControlUpdateCounter(csw_ptr);

				if (csw_state == 0)		// Waiting for tone
				{
					DCDI=1;
					TapeAudio.Signal=0;
				}
				
				// New data read in, so do something about it
				if (csw_state == 1)		// In tone
				{
					DCDI=1;
					TapeAudio.Signal=2;
					TapeAudio.BytePos=11;
				}

				if ( (CSW_BUF >= 0) && (csw_state == 2) )
				{
					DCDI=0;
					HandleData(CSW_BUF);
					TapeAudio.Data=(CSW_BUF<<1)|1;
					TapeAudio.BytePos=1;
					TapeAudio.CurrentBit=0;
					TapeAudio.Signal=1;
					TapeAudio.ByteCount=3;
				}
			}
			if ((Cass_Relay==1) && (RxD<2) && CSWOpen)
			{
				if (TotalCycles >= TapeTrigger)
				{
					if (TapePlaying)
						TapeClock++;
					
					TapeTrigger = TotalCycles + CSW_CYCLES;
					
				}
			}
			
			if (DCDI==1 && ODCDI==0)
			{
				// low to high transition on the DCD line
				if (RIE)
				{
					intStatus|=1<<serial;
					SetACIAStatus(7);
				}
				DCD=1; SetACIAStatus(2); //ResetACIAStatus(0);
				DCDClear=0;
			}
			if (DCDI==0 && ODCDI==1)
			{
				DCD=0; ResetACIAStatus(2);
				DCDClear=0;
			}
			if (DCDI!=ODCDI)
				ODCDI=DCDI;
		}
	}

}

void CloseUEF(void) {
	if (UEFOpen) {
		TapeControlStopRecording(false);
		uef_close();
		UEFOpen=0;
		TxD=0;
		RxD=0;
		if (TapeControlEnabled)
		{
#if 0 //ACH - tape close
			const ControlID dbControlID = { 'SLST', 0 };
			ControlRef dbControl;
			
			GetControlByID (mTCWindow, &dbControlID, &dbControl);
			RemoveDataBrowserItems(dbControl, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);
#else
            swift_UpdateItem("clearallitems", 0);
#endif
            
        }
		
	}
}

void Kill_Serial(void) {
	CloseUEF();
	CloseCSW();

//	if (SerialPortOpen)
//		CloseHandle(hSerialPort);

}


void LoadUEF(char *UEFName) {
	CloseUEF();

	strcpy(UEFTapeName, UEFName);

	if (TapeControlEnabled)
		TapeControlOpenFile(UEFName);

	// Clock values:
	// 5600 - Normal speed - anything higher is a bit slow
	// 750 - Recommended minium settings, fastest reliable load
	uef_setclock(TapeClockSpeed);
	SetUnlockTape(UnlockTape);

	if (uef_open(UEFName)) {
		UEFOpen=1;
		UEF_BUF=0;
		TxD=0;
		RxD=0;
		TapeClock=0;
		OldClock=0;
		TapeTrigger=TotalCycles+TAPECYCLES;
		TapeControlUpdateCounter(TapeClock);
	}
	else {
		UEFTapeName[0]=0;
	}
}

void RewindTape(void) {
	TapeControlStopRecording(true);
	UEF_BUF=0;
	TapeClock=0;
	OldClock=0;
	TapeTrigger=TotalCycles+TAPECYCLES;
	TapeControlUpdateCounter(TapeClock);

	csw_state = 0;
	csw_bit = 0;
	csw_pulselen = 0;
	csw_ptr = 0;
	csw_pulsecount = -1;
	
}

void SetTapeSpeed(int speed) {
	int NewClock = (int)((double)TapeClock * ((double)speed / TapeClockSpeed));
	TapeClockSpeed=speed;
	if (UEFOpen)
		LoadUEF(UEFTapeName);
	TapeClock=NewClock;
}

void SetUnlockTape(int unlock) {
	uef_setunlock(unlock);
}

//*******************************************************************

bool map_file(char *file_name)
{
	bool done=false;
	int file;
	int i;
	int start_time;
	int n;
	int data;
	int last_data;
	int blk;
	int blk_num;
	char block[500];
	bool std_last_block=true;
	char name[11];

	uef_setclock(TapeClockSpeed);

	file = uef_open(file_name);
	if (file == 0)
	{
		return(false);
	}

	i=0;
	start_time=0;
	map_lines=0;
	last_data=0;
	blk_num=0;

	memset(map_desc, 0, sizeof(map_desc));
	memset(map_time, 0, sizeof(map_time));
	
	while (!done && map_lines < MAX_MAP_LINES)
	{
		data = uef_getdata(i);
		if (data != last_data)
		{
			if (UEFRES_TYPE(data) != UEFRES_TYPE(last_data))
			{
				if (UEFRES_TYPE(last_data) == UEF_DATA)
				{
					// End of block, standard header?
					if (blk > 20 && block[0] == 0x2A)
					{
						if (!std_last_block)
						{
							// Change of block type, must be first block
							blk_num=0;
							if (map_lines > 0 && map_desc[map_lines-1][0] != 0)
							{
								strcpy(map_desc[map_lines], "");
								map_time[map_lines]=start_time;
								map_lines++;
							}
						}

						// Pull file name from block
						n = 1;
						while (block[n] != 0 && block[n] >= 32 && block[n] <= 127 && n <= 10)
						{
							name[n-1] = block[n];
							n++;
						}
						name[n-1] = 0;
						if (name[0] != 0)
							sprintf(map_desc[map_lines], "%-12s %02X  Length %04X", name, blk_num, blk);
						else
							sprintf(map_desc[map_lines], "<No name>    %02X  Length %04X", blk_num, blk);

						map_time[map_lines]=start_time;

						// Is this the last block for this file?
						if (block[strlen(name) + 14] & 0x80)
						{
							blk_num=-1;
							++map_lines;
							strcpy(map_desc[map_lines], "");
							map_time[map_lines]=start_time;
						}
						std_last_block=true;
					}
					else
					{
						sprintf(map_desc[map_lines], "Non-standard %02X  Length %04X", blk_num, blk);
						map_time[map_lines]=start_time;
						std_last_block=false;
					}

					// Replace time counter in previous blank lines
					if (map_lines > 0 && map_desc[map_lines-1][0] == 0)
						map_time[map_lines-1]=start_time;

					// Data block recorded
					map_lines++;
					blk_num++;
				}
					
				if (UEFRES_TYPE(data) == UEF_HTONE)
				{
					// strcpy(map_desc[map_lines++], "Tone");
					start_time=i;
				}
				else if (UEFRES_TYPE(data) == UEF_GAP)
				{
					if (map_lines > 0 && map_desc[map_lines-1][0] != 0)
						strcpy(map_desc[map_lines++], "");
					start_time=i;
					blk_num=0;
				}
				else if (UEFRES_TYPE(data) == UEF_DATA)
				{
					blk=0;
					block[blk++]=UEFRES_BYTE(data);
				}
				else if (UEFRES_TYPE(data) == UEF_EOF)
				{
					done=true;
				}
			}
			else
			{
				if (UEFRES_TYPE(data) == UEF_DATA)
				{
					if (blk < 500)
						block[blk++]=UEFRES_BYTE(data);
					else
						blk++;
				}
			}
		}
		last_data=data;
		i++;
	}

	uef_close();

//	for (i = 0; i < map_lines; ++i)
//		fprintf(stderr, "%s\n", map_desc[i]);
	
	return(true);
}
#if 0 //ACH - tape control

//*******************************************************************

OSStatus TCWindowCommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    HICommand command;
    OSStatus err = noErr;
    err = GetEventParameter(event, kEventParamDirectObject,
							typeHICommand, NULL, sizeof(HICommand), NULL, &command);
    require_noerr (err, CantGetParameter);
	
	err = noErr;
	switch (command.commandID)
#else

//swift
OSStatus TCWindowCommandHandler(UInt32 cmdID)
{
    OSStatus err = noErr;

    switch (cmdID)
#endif

    {
        case 'tppl':
            fprintf(stderr, "Tape Control Play selected\n");
			TapePlaying=true;
			TapeControlStopRecording(true);
			TapeAudio.Enabled=Cass_Relay?TRUE:FALSE;
            break;
        case 'tpst':
            fprintf(stderr, "Tape Control Stop selected\n");
			TapePlaying=false;
			TapeControlStopRecording(true);
			TapeAudio.Enabled=FALSE;
            break;
        case 'tpej':
            fprintf(stderr, "Tape Control Eject selected\n");
			TapeControlStopRecording(false);
			TapeAudio.Enabled=FALSE;
			CloseUEF();
			CloseCSW();
            break;
        case 'tprc':
            fprintf(stderr, "Tape Control Record selected\n");

			SInt16 r;
			Str255 S1;
			Str255 S2;
			
			if (CSWOpen) break;
				
			AlertStdAlertParamRec alertParameters;
			
			alertParameters.movable			= true;
			alertParameters.helpButton		= false;
			alertParameters.filterProc		= NULL;
			alertParameters.defaultText		= (StringPtr)kAlertDefaultOKText;
			alertParameters.cancelText		= (StringPtr)kAlertDefaultCancelText;
			alertParameters.otherText		= NULL;
			alertParameters.defaultButton	= kAlertStdAlertOKButton;
			alertParameters.cancelButton	= kAlertStdAlertCancelButton;
			alertParameters.position		= kWindowDefaultPosition;

			if (!TapeRecording)
			{
				r = 2;				// Cancel
				if (UEFOpen)
				{

#if 0 //ACH
					CopyCStringToPascal("Append to current tape file :", S1);
					CopyCStringToPascal(UEFTapeName, S2);
					
					StandardAlert( kAlertNoteAlert, S1, S2, &alertParameters, &r);
					
					if (r == 1)		// OK
					{
						// SendMessage(hwndMap, LB_SETCURSEL, (WPARAM)map_lines-1, 0);
						
						const ControlID dbControlID = { 'SLST', 0 };
						ControlRef dbControl;
						
						GetControlByID (mTCWindow, &dbControlID, &dbControl);
						SetDataBrowserSelectedItems (dbControl, 1, (DataBrowserItemID *) &map_lines, kDataBrowserItemsAssign);
						
					}
#else
                    // http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/databrow_reference/databrowser_ref/constant_21.html#//apple_ref/doc/uid/TP30000969-CH202-C009004
                    
                    r = swift_Alert("Append to current tape file :",UEFTapeName,true);
                    // select item number 1 from all the items in maplist
                    if (r == 1)// OK
                        swift_SelectItem("SLST_assign", 1,(DataBrowserItemID *) &map_lines);
#endif
				}
				else
				{
					// Query for new file name
					CloseUEF();
					mainWin->NewTapeImage(UEFTapeName);
					if (UEFTapeName[0])
					{
						r = 1;
						FILE *fd = fopen(UEFTapeName,"rb");
						if (fd != NULL)
						{
							fclose(fd);

#if 0 //ACH
							CopyCStringToPascal("File already exists", S1);
							CopyCStringToPascal("Overwrite file ?", S2);
							StandardAlert( kAlertNoteAlert, S1, S2, &alertParameters, &r);
#else
                            r = swift_Alert("File already exists","Overwrite file ?",true);
#endif
						}
						
						if (r == 1)		// OK
						{
							// Create file
							if (uef_create(UEFTapeName))
							{
								UEFOpen=1;
							}
							else
							{
#if 0 //ACH

								CopyCStringToPascal("Error creating tape file:", S1);
								CopyCStringToPascal(UEFTapeName, S2);
								alertParameters.cancelButton	= 0;
								StandardAlert( kAlertNoteAlert, S1, S2, &alertParameters, &r);
#else
                                r = swift_Alert("Error creating tape file:", UEFTapeName, false);
#endif
								UEFTapeName[0] = 0;
								r = 2;			// Cancel
							}
						}
					}
				}
				
				if (r == 1)		// OK
				{
					TapeRecording=true;
					TapePlaying=false;
					TapeAudio.Enabled=Cass_Relay?TRUE:FALSE;
				}
			}
			
			break;
			
        default:
            err = eventNotHandledErr;
            break;
    }
	
CantGetParameter:
		return err;
}

#if 0 //ACH -- catch window close, see beeb_TapeControlCloseDialog
static OSStatus TCWindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    OSStatus err = noErr;
    switch (GetEventKind(event))
    {
        case kEventWindowClosed: 
			mTCWindow = NULL;
			TapeControlEnabled = FALSE;
			map_lines = 0;
			TapePlaying=true;
			TapeRecording=false;
            break;
        default:
            err = eventNotHandledErr;
            break;
    }
    
    return err;
}

OSStatus TCWindowCallback(ControlRef browser, DataBrowserItemID itemID, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean changeValue)
{
OSStatus status = noErr;
char temp[256];
CFStringRef pTitle;
	
//	fprintf(stderr, "Item = %08x, Property = %08x, change = %d\n", itemID, property, changeValue);
	
	if (!changeValue)
	{
		switch(property)
		{
			case 'NAME' :
				
				strcpy(temp, map_desc[itemID - 1]);
				temp[12] = 0;
				pTitle = CFStringCreateWithCString (kCFAllocatorDefault, temp, kCFStringEncodingASCII);
				status = SetDataBrowserItemDataText(itemData, pTitle);
				CFRelease(pTitle);
				
				break;

			case 'BLCK' :
				
				strcpy(temp, map_desc[itemID - 1] + 13);
				temp[2] = 0;
				pTitle = CFStringCreateWithCString (kCFAllocatorDefault, temp, kCFStringEncodingASCII);
				status = SetDataBrowserItemDataText(itemData, pTitle);
				CFRelease(pTitle);
				
				break;

			case 'LENG' :
				
				strcpy(temp, map_desc[itemID - 1] + 16);
				pTitle = CFStringCreateWithCString (kCFAllocatorDefault, temp, kCFStringEncodingASCII);
				status = SetDataBrowserItemDataText(itemData, pTitle);
				CFRelease(pTitle);
				
				break;
				
			case 3 :
				int s;
				s = itemID - 1;
				if (s >= 0 && s < map_lines)
				{
					if (CSWOpen)
					{
						csw_ptr = map_time[s];
					}
					else
					{
						TapeClock=map_time[s];
					}
					OldClock=0;
					TapeTrigger=TotalCycles+TAPECYCLES;
				}
				break;
				
			default:
				status = errDataBrowserPropertyNotSupported;
				break;
		}
	}
	else
		status = errDataBrowserPropertyNotSupported;

	return status;
}

	
void TapeControlOpenDialog()
{
	int Clock;

	TapeControlEnabled = TRUE;

	IBNibRef 		nibRef;
	EventTypeSpec TCcommands[] = {
	{ kEventClassCommand, kEventCommandProcess }
	};
		
	EventTypeSpec TCevents[] = {
	{ kEventClassWindow, kEventWindowClosed }
	};

	if (mTCWindow == NULL)
	{
		// Create a Nib reference passing the name of the nib file (without the .nib extension)
		// CreateNibReference only searches into the application bundle.
		CreateNibReference(CFSTR("main"), &nibRef);
		CreateWindowFromNib(nibRef, CFSTR("Window"), &mTCWindow);
		DisposeNibReference(nibRef);
		ShowWindow(mTCWindow);
		
		InstallWindowEventHandler(mTCWindow, 
							  NewEventHandlerUPP (TCWindowCommandHandler), 
							  GetEventTypeCount(TCcommands), TCcommands, 
							  mTCWindow, NULL);
		
		InstallWindowEventHandler (mTCWindow, 
								   NewEventHandlerUPP (TCWindowEventHandler), 
								   GetEventTypeCount(TCevents), TCevents, 
								   mTCWindow, NULL);
		
		const ControlID dbControlID = { 'SLST', 0 };
		ControlRef dbControl;
		DataBrowserCallbacks dbCallbacks;
		
		GetControlByID (mTCWindow, &dbControlID, &dbControl);
		dbCallbacks.version = kDataBrowserLatestCallbacks;
		InitDataBrowserCallbacks(&dbCallbacks);
		dbCallbacks.u.v1.itemDataCallback =
			NewDataBrowserItemDataUPP( (DataBrowserItemDataProcPtr) TCWindowCallback);
		SetDataBrowserCallbacks(dbControl, &dbCallbacks);
		SetAutomaticControlDragTrackingEnabledForWindow(mTCWindow, true);
#else

extern "C" void beeb_TapeControlOpenDialog()
{
    int Clock;

    TapeControlEnabled = TRUE;

    if (true)
    {
#endif

		if (UEFOpen)
		{
			Clock = TapeClock;
			LoadUEF(UEFTapeName);
			TapeClock = Clock;
			TapeControlUpdateCounter(TapeClock);
		}

		if (CSWOpen)
		{
			Clock = csw_ptr;
			LoadCSW(UEFTapeName);
			csw_ptr = Clock;
			TapeControlUpdateCounter(csw_ptr);
		}
		
		
	}
}

#if 0 //ACH
void TapeControlCloseDialog()
{
	if (mTCWindow)
	{
		HideWindow(mTCWindow);
		DisposeWindow(mTCWindow);
	}
	mTCWindow = NULL;
#else
extern "C" void beeb_TapeControlCloseDialog()
{
#endif
	TapeControlEnabled = FALSE;
	map_lines = 0;
	TapePlaying=true;
	TapeRecording=false;
}

void TapeControlOpenFile(char *UEFName)
{
	if (TapeControlEnabled) 
	{
		if (CSWOpen == 0)
		{
			if (!map_file(UEFName))
			{
				char errstr[256];
				sprintf(errstr, "Cannot open UEF file:\n  %s", UEFName);
				return;
			}
		}
#if 0 //ACH - tape openfile
		const ControlID dbControlID = { 'SLST', 0 };
		ControlRef dbControl;
			
		GetControlByID (mTCWindow, &dbControlID, &dbControl);
		RemoveDataBrowserItems(dbControl, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);
		AddDataBrowserItems(dbControl, kDataBrowserNoItem, map_lines, NULL, kDataBrowserItemNoProperty);
#else
        swift_UpdateItem("SLST_clear", map_lines);
#endif

		TapeControlUpdateCounter(0);

	}
}

void TapeControlUpdateCounter(int tape_time)
{
	int i, j;

	if (TapeControlEnabled) 
	{
		i = 0;
		while (i < map_lines && map_time[i] <= tape_time)
			i++;

		if (i > 0)
			i--;

		//		SendMessage(hwndMap, LB_SETCURSEL, (WPARAM)i, 0);
#if 0 //ACH - tape updatecounter

		const ControlID dbControlID = { 'SLST', 0 };
		ControlRef dbControl;
		
		GetControlByID (mTCWindow, &dbControlID, &dbControl);
		j = i + 1;
		SetDataBrowserSelectedItems (dbControl, 1, (DataBrowserItemID *) &j, kDataBrowserItemsAssign);
		RevealDataBrowserItem(dbControl, j, kDataBrowserNoItem, kDataBrowserRevealOnly);
#else
        j = i + 1;
        swift_SelectItem("SLST_reveal", j, NULL);
#endif
        
	}
}

void TapeControlStopRecording(bool RefreshControl)
{
	if (TapeRecording)
	{
		uef_putdata(UEF_EOF, 0);
		TapeRecording = false;

		if (RefreshControl)
		{
			LoadUEF(UEFTapeName);
		}
	}
}

//*******************************************************************

void SaveSerialUEF(FILE *SUEF)
{
	if (UEFOpen)
	{
		fput16(0x0473,SUEF);
		fput32(293,SUEF);
		fputc(SerialChannel,SUEF);
		fwrite(UEFTapeName,1,256,SUEF);
		fputc(Cass_Relay,SUEF);
		fput32(Tx_Rate,SUEF);
		fput32(Rx_Rate,SUEF);
		fputc(Clk_Divide,SUEF);
		fputc(Parity,SUEF);
		fputc(Stop_Bits,SUEF);
		fputc(Data_Bits,SUEF);
		fputc(RIE,SUEF);
		fputc(TIE,SUEF);
		fputc(TxD,SUEF);
		fputc(RxD,SUEF);
		fputc(RDR,SUEF);
		fputc(TDR,SUEF);
		fputc(RDSR,SUEF);
		fputc(TDSR,SUEF);
		fputc(ACIA_Status,SUEF);
		fputc(ACIA_Control,SUEF);
		fputc(SP_Control,SUEF);
		fputc(DCD,SUEF);
		fputc(DCDI,SUEF);
		fputc(ODCDI,SUEF);
		fputc(DCDClear,SUEF);
		fput32(TapeClock,SUEF);
		fput32(TapeClockSpeed,SUEF);
	}
}

void LoadSerialUEF(FILE *SUEF)
{
	char FileName[256];
	int sp;

	CloseUEF();

	SerialChannel=fgetc(SUEF);
	fread(FileName,1,256,SUEF);
	if (FileName[0])
	{
		LoadUEF(FileName);
		if (!UEFOpen)
		{
			if (!TapeControlEnabled)
			{
				fprintf(stderr, "Cannot open UEF file:\n  %s\n", FileName);
			}
		}
		else
		{
			Cass_Relay=fgetc(SUEF);
			Tx_Rate=fget32(SUEF);
			Rx_Rate=fget32(SUEF);
			Clk_Divide=fgetc(SUEF);
			Parity=fgetc(SUEF);
			Stop_Bits=fgetc(SUEF);
			Data_Bits=fgetc(SUEF);
			RIE=fgetc(SUEF);
			TIE=fgetc(SUEF);
			TxD=fgetc(SUEF);
			RxD=fgetc(SUEF);
			RDR=fgetc(SUEF);
			TDR=fgetc(SUEF);
			RDSR=fgetc(SUEF);
			TDSR=fgetc(SUEF);
			ACIA_Status=fgetc(SUEF);
			ACIA_Control=fgetc(SUEF);
			SP_Control=fgetc(SUEF);
			DCD=fgetc(SUEF);
			DCDI=fgetc(SUEF);
			ODCDI=fgetc(SUEF);
			DCDClear=fgetc(SUEF);
			TapeClock=fget32(SUEF);
			sp=fget32(SUEF);
			if (sp != TapeClockSpeed)
			{
				TapeClock = (int)((double)TapeClock * ((double)TapeClockSpeed / sp));
			}
			TapeControlUpdateCounter(TapeClock);
		}
	}
}

extern "C" long beeb_TCHandleCommand(unsigned int cmdID)
{
    char* cmdCHR = (char*)&cmdID;
    printf("%c%c%c%c", cmdCHR[3], cmdCHR[2], cmdCHR[1], cmdCHR[0]);
    return TCWindowCommandHandler(cmdID);
}
    
extern "C" long beeb_getTableRowsCount(const char* tablename)
{
    if (UEFOpen)
        return map_lines;
    return 0;
}

char temp[256];

extern "C" const char* beeb_getTableCellData(UInt32 property, long itemID)
{
    char* propertyCHR = (char*)&property;

//    printf("%c%c%c%c data %ld", propertyCHR[3], propertyCHR[2], propertyCHR[1], propertyCHR[0], itemID);
    
    switch(property)
    {
        case 'NAME' :
            
            strcpy(temp, map_desc[itemID - 1]);
            temp[12] = 0;
            
            break;

        case 'BLCK' :
            
            strcpy(temp, map_desc[itemID - 1] + 13);
            temp[2] = 0;
            
            break;

        case 'LENG' :
            
            strcpy(temp, map_desc[itemID - 1] + 16);
            
            break;
            
        case 3 :
            long s;
            s = itemID - 1;
            if (s >= 0 && s < map_lines)
            {
                if (CSWOpen)
                {
                    csw_ptr = map_time[s];
                }
                else
                {
                    TapeClock=map_time[s];
                }
                OldClock=0;
                TapeTrigger=TotalCycles+TAPECYCLES;
            }
            break;
    }
    return temp;
}

