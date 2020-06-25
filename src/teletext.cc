/* Teletext Support for Beebem */

/*

Offset  Description                 Access  
+00     data						R/W  
+01     read status                 R  
+02     write select                W  
+03     write irq enable            W  


*/

#include <stdio.h>
#include <stdlib.h>
#include "teletext.h"
#include "debug.h"
#include "6502core.h"
#include "main.h"
#include "beebmem.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef int	SOCKET;
#define SOCKET_ERROR	-1
#define INVALID_SOCKET	-1

char TeleTextAdapterEnabled = 0;
char TeleTextData = 0;
char TeleTextServer = 0;
int TeleTextStatus = 0xef;
bool TeleTextInts = false;
int rowPtrOffset = 0x00;
int rowPtr = 0x00;
int colPtr = 0x00;

FILE *txtFile = NULL;
long txtFrames = 0;
long txtCurFrame = 0;
int txtChnl = 0;

unsigned char row[16][43];

SOCKET txtListenSocket = 0;		// Listen socket
sockaddr_in txtServer;

void TeleTextLog(const char *text, ...)
{
va_list argptr;

	return;
	
	va_start(argptr, text);
    vfprintf(stderr, text, argptr);
	va_end(argptr);
}

void TeleTextInit(void)

{
char buff[256];

    TeleTextStatus = 0xef;

    rowPtr = 0x00;
    colPtr = 0x00;

    txtCurFrame = 0;

    if (txtFile) fclose(txtFile);

	if (!TeleTextAdapterEnabled)
		return;
	
	sprintf(buff, "%s/diskimg/txt%d.dat", RomPath, txtChnl);
	
    if (TeleTextData)
	{
		txtFile = fopen(buff, "rb");
		if (txtFile)
		{
			fseek(txtFile, 0L, SEEK_END);
			txtFrames = ftell(txtFile) / 860L;
			fseek(txtFile, 0L, SEEK_SET);
		}
	
		TeleTextLog("TeleTextInit Frames = %ld\n", txtFrames);
	}
	
	if (txtListenSocket != 0)
	{
		close(txtListenSocket);
	}
	
    if (TeleTextServer)
	{
		txtListenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (txtListenSocket == SOCKET_ERROR)
		{
			TeleTextLog("TeleTextInit Socket Error\n");
		}
	
		txtServer.sin_family = AF_INET;
		txtServer.sin_addr.s_addr = INADDR_ANY;
		txtServer.sin_port = htons(9999);
	
		if (bind(txtListenSocket, (struct sockaddr*) &txtServer, sizeof(txtServer)) == SOCKET_ERROR)
		{
			TeleTextLog("TeleTextInit Bind Error\n");
		}
		else
		{
			TeleTextLog("Socket Ready to Go !\n");
		}
	}
}

void TeleTextWrite(int Address, int Value) 
{

	if (!TeleTextAdapterEnabled)
		return;

    TeleTextLog("TeleTextWrite Address = 0x%02x, Value = 0x%02x, PC = 0x%04x\n", Address, Value, ProgramCounter);
	
    switch (Address)
    {
		case 0x00:
            if ( (Value & 0x0c) == 0x0c)
            {
                TeleTextInts = true;
            }
            if ( (Value & 0x0c) == 0x00) TeleTextInts = false;
            if ( (Value & 0x10) == 0x00) TeleTextStatus &= ~0x10;			// Clear data available
			
			if ( (Value & 0x03) != txtChnl)
			{
				txtChnl = Value & 0x03;
				TeleTextInit();
			}
			break;

		case 0x01:
            rowPtr = Value;
            colPtr = 0x00;
            break;
		case 0x02:
            row[rowPtr][colPtr++] = Value;
            break;
		case 0x03:
            TeleTextStatus &= ~0x10;			// Clear data available
			break;
    }
}

int TeleTextRead(int Address)
{

	if (!TeleTextAdapterEnabled)
		return 0xff;
	
int data = 0x00;

    switch (Address)
    {
    case 0x00 :         // Data Register
        data = TeleTextStatus;
   		intStatus&=~(1<<teletext);
        break;
    case 0x01:			// Status Register
        break;
    case 0x02:

        if (colPtr == 0x00)
            TeleTextLog("TeleTextRead Reading Row %d, PC = 0x%04x\n", 
                rowPtr, ProgramCounter);

        if (colPtr >= 43)
        {
            TeleTextLog("TeleTextRead Reading Past End Of Row %d, PC = 0x%04x\n", rowPtr, ProgramCounter);
            colPtr = 0;
        }

//        TeleTextLog("TeleTextRead Returning Row %d, Col %d, Data %d, PC = 0x%04x\n", 
//            rowPtr, colPtr, row[rowPtr][colPtr], ProgramCounter);
        
        data = row[rowPtr][colPtr++];

        break;

    case 0x03:
        break;
    }

    TeleTextLog("TeleTextRead Address = 0x%02x, Value = 0x%02x, PC = 0x%04x\n", Address, data, ProgramCounter);
    
    return data;
}

void TeleTextPoll(void)

{
int i;
char buff[13 * 43];
fd_set RdFds;
timeval TmOut = {0, 0};
char rxBuff[1024];
int RetVal;

	if (!TeleTextAdapterEnabled)
		return;

	TeleTextStatus |= 0x10;			// teletext data available

	if (TeleTextServer)
	{
		if (txtListenSocket)
		{
			FD_ZERO(&RdFds);
			FD_SET(txtListenSocket, &RdFds);
			RetVal = select(32, &RdFds, NULL, NULL, &TmOut);
			if (RetVal > 0)
			{
				RetVal = recv(txtListenSocket, (char *) rxBuff, sizeof(rxBuff), 0);
				if (RetVal > 0)
				{
				
//					TeleTextLog("TeleTextPoll Read Packet Of Length %d\n", RetVal);
				
					if (TeleTextInts == true)
					{
						intStatus|=1<<teletext;
					
						rowPtr = 0x00;
						colPtr = 0x00;
					
						for (i = 0; i < 16; ++i)
						{
							switch(i)
							{
								case 0 :
								case 14 :
								case 15 :
									row[i][0] = 0x00;
									break;
								default :
									row[i][0] = 0x67;
									memcpy(&(row[i][1]), rxBuff + (3 + i - 1) * 43, 42);
							}
						}
					
					}
				}
			}
			else
			{
				TeleTextLog("TeleTextPoll No Data\n");
			}
		
			return;
		}
	}
	
	if (TeleTextData)
	{
		if (txtFile)
		{

			if (TeleTextInts == true)
			{

				intStatus|=1<<teletext;

//				TeleTextStatus = 0xef;
				rowPtr = 0x00;
				colPtr = 0x00;

				TeleTextLog("TeleTextPoll Reading Frame %ld, PC = 0x%04x\n", txtCurFrame, ProgramCounter);

				fseek(txtFile, txtCurFrame * 860L + 3L * 43L, SEEK_SET);
				fread(buff, 13 * 43, 1, txtFile);
				for (i = 0; i < 16; ++i)
				{
					switch(i)
					{
						case 0 :
						case 14 :
						case 15 :
							row[i][0] = 0x00;
							break;
						default :
							row[i][0] = 0x67;
							memcpy(&(row[i][1]), buff + (i - 1) * 43, 42);
					}
				}
        
				txtCurFrame++;
				if (txtCurFrame >= txtFrames) txtCurFrame = 0;
			}
		}
	}
}
