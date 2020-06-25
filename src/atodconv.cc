/****************************************************************************/
/*                               Beebem                                     */
/*                               ------                                     */
/* This program may be distributed freely within the following restrictions:*/
/*                                                                          */
/* 1) You may not charge for this program or for any part of it.            */
/* 2) This copyright message must be distributed with all copies.           */
/* 3) This program must be distributed complete with source code.  Binary   */
/*    only distribution is not permitted.                                   */
/* 4) The author offers no warrenties, or guarentees etc. - you use it at   */
/*    your own risk.  If it messes something up or destroys your computer   */
/*    thats YOUR problem.                                                   */
/* 5) You may use small sections of code from this program in your own      */
/*    applications - but you must acknowledge its use.  If you plan to use  */
/*    large sections then please ask the author.                            */
/*                                                                          */
/* If you do not agree with any of the above then please do not use this    */
/* program.                                                                 */
/****************************************************************************/
/* Analogue to digital converter support file for the beeb emulator -
   Mike Wyatt 7/6/97 */

#include <stdio.h>

#include "6502core.h"
#include "atodconv.h"
#include "sysvia.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "uefstate.h"


int JoystickEnabled = 0;

/* X and Y positions for joystick 1 */
int JoystickX;
int JoystickY;

/* A to D state */
typedef struct AtoDStateT {
	unsigned char datalatch;
	unsigned char status;
	unsigned char high;
	unsigned char low;
} AtoDStateT;

AtoDStateT AtoDState;

int AtoDTrigger;  /* For next A to D conversion completion */

/*--------------------------------------------------------------------------*/
/* Address is in the range 0-f - with the fec0 stripped out */
void AtoDWrite(int Address, int Value)
{

	if (Address == 0)
	{
		int timetoconvert;
		AtoDState.datalatch = Value & 0xff;
		if (AtoDState.datalatch & 8)
			timetoconvert = 20000; /* 10 bit conversion, 10 ms */
		else
			timetoconvert = 8000; /* 8 bit conversion, 4 ms */
		SetTrigger(timetoconvert,AtoDTrigger);

		AtoDState.status = (AtoDState.datalatch & 0xf) | 0x80; /* busy, not complete */
	}
}

/*--------------------------------------------------------------------------*/
/* Address is in the range 0-f - with the fec0 stripped out */
int AtoDRead(int Address)
{
	int Value = 0xff;

	switch (Address)
	{
	case 0:
		Value = AtoDState.status;
		break;

	case 1:
		Value = AtoDState.high;
		break;

	case 2:
		Value = AtoDState.low;
		break;
	}

	return(Value);
}

/*--------------------------------------------------------------------------*/
void AtoD_poll_real(void)
{
	int value;

	ClearTrigger(AtoDTrigger);
	AtoDState.status &= 0xf;
	AtoDState.status |= 0x40; /* not busy */
	PulseSysViaCB1();

	switch (AtoDState.status & 3)
	{
	case 0:
		value = JoystickX;
		break;
	case 1:
		value = JoystickY;
		break;
	default:
		value = 0;
		break;
	}

	AtoDState.status |= (value & 0xc000)>>10;
	AtoDState.high = value>>8;
	AtoDState.low = value & 0xf0;
}

/*--------------------------------------------------------------------------*/
void AtoDInit(void)
{
	AtoDState.datalatch = 0;
	AtoDState.high = 0;
	AtoDState.low = 0;
	ClearTrigger(AtoDTrigger);
	
	/* Move joystick to middle */
	JoystickX = 32767;
	JoystickY = 32767;
	
	/* Not busy, conversion complete (OS1.2 will then request another conversion) */
	AtoDState.status = 0x40;
	PulseSysViaCB1();
}

/*--------------------------------------------------------------------------*/
void AtoDEnable(void)
{
	JoystickEnabled = 1;
	AtoDInit();
}

/*--------------------------------------------------------------------------*/
void AtoDDisable(void)
{
	JoystickEnabled = 0;
	AtoDState.datalatch = 0;
	AtoDState.status = 0x80; /* busy, conversion not complete */
	AtoDState.high = 0;
	AtoDState.low = 0;
	ClearTrigger(AtoDTrigger);
	
	/* Move joystick to middle (superpool looks at joystick even when not selected) */
	JoystickX = 32767;
	JoystickY = 32767;
}

/*--------------------------------------------------------------------------*/
void SaveAtoDUEF(FILE *SUEF) {
	fput16(0x0474,SUEF);
	fput32(8,SUEF);
	fputc(AtoDState.datalatch,SUEF);
	fputc(AtoDState.status,SUEF);
	fputc(AtoDState.high,SUEF);
	fputc(AtoDState.low,SUEF);
	if (AtoDTrigger == CycleCountTMax)
		fput32(AtoDTrigger,SUEF);
	else
		fput32(AtoDTrigger - TotalCycles,SUEF);
}
void LoadAtoDUEF(FILE *SUEF) {
	AtoDState.datalatch = fgetc(SUEF);
	AtoDState.status = fgetc(SUEF);
	AtoDState.high = fgetc(SUEF);
	AtoDState.low = fgetc(SUEF);
	AtoDTrigger = fget32(SUEF);
	if (AtoDTrigger != CycleCountTMax)
		AtoDTrigger+=TotalCycles;
}
