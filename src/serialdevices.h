/*
 *  serialdevices.h
 *  BeebEm3
 *
 *  Created by Jon Welch on 28/08/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

extern unsigned char TouchScreenEnabled;
extern unsigned char EthernetPortEnabled;

#if 0 //ACH - serial/ethernet windows
extern WindowRef mSerialPortWindow;
extern WindowRef mEthernetPortWindow;
#endif
extern int mSerialHandle;
extern int mEthernetHandle;

void EthernetPortOpenDialog();
void EthernetPortCloseDialog();
#if 0 //ACH - ethernet thread
OSStatus MyEthernetPortStatusThread(void *parameter);
OSStatus MyEthernetPortReadThread(void *parameter);
OSStatus MyListenThread(void *parameter);
#endif
void EthernetPortOpen(void);
bool EthernetPortPoll(void);
void EthernetPortClose(void);
void EthernetPortWrite(unsigned char data);
unsigned char EthernetPortRead(void);
void EthernetPortStore(unsigned char data);
unsigned char EthernetPortGet(void);

void TouchScreenOpen(void);
bool TouchScreenPoll(void);
void TouchScreenClose(void);
void TouchScreenWrite(unsigned char data);
unsigned char TouchScreenRead(void);
void TouchScreenStore(unsigned char data);
void TouchScreenReadScreen(bool check);

void SerialPortOpenDialog();
void SerialPortCloseDialog();
int OpenSerialPort(const char *bsdPath);
void CloseSerialPort(int fileDescriptor);
#if 0 //ACH - serial thread
OSStatus MySerialReadThread(void *parameter);
OSStatus MySerialStatusThread(void *parameter);
#endif
void SetSerialPortFormat(int Data_Bits, int Stop_Bits, int Parity, int RTS);
void SetSerialPortBaud(int Tx_Rate, int Rx_Rate);
void SerialPortWrite(unsigned char TDR);
unsigned char SerialPortGetChar(void);
int SerialPortIsChar(void);
