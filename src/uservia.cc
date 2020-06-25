/****************************************************************************/
/*              Beebem - (c) David Alan Gilbert 1994                        */
/*              ------------------------------------                        */
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
/* Please report any problems to the author at beebem@treblig.org           */
/****************************************************************************/
/* User VIA support file for the beeb emulator - David Alan Gilbert 11/12/94 */
/* Modified from the system via */

#include <stdio.h>

#include "6502core.h"
#include "uservia.h"
#include "sysvia.h"
#include "via.h"
#include "viastate.h"
#include "debug.h"
#include "main.h"
#include "tube.h"

#ifdef WIN32
#include <windows.h>
#include "main.h"
#endif

int RTC_bit = 0;
int RTC_cmd = 0;
int RTC_data = 0;        // Mon    Yr   Day         Hour        Min
unsigned char RTC_ram[8] = {0x12, 0x01, 0x05, 0x00, 0x05, 0x00, 0x07, 0x00};

int RTC_Enabled = 0;

WindowRef mBreakOutWindow = NULL; 

int	BitKey;			// Used to store the bit key pressed while we wait 
int BitKeys[8] = {29, 18, 19, 20, 21, 23, 22, 26};
int LastBitButton = 0;

/* AMX mouse (see uservia.h) */
int AMXMouseEnabled = 0;
int AMXLRForMiddle = 0;
int AMXTrigger = 0;
int AMXButtons = 0;
int AMXTargetX = 0;
int AMXTargetY = 0;
int AMXCurrentX = 0;
int AMXCurrentY = 0;

/* Printer Port */
int PrinterEnabled = 0;
int PrinterTrigger = 0;
static char PrinterFileName[256];
FILE *PrinterFileHandle = NULL;

extern int DumpAfterEach;
/* My raw VIA state */
VIAState UserVIAState;

/*--------------------------------------------------------------------------*/
static void UpdateIFRTopBit(void) {
  /* Update top bit of IFR */
  if (UserVIAState.ifr&(UserVIAState.ier&0x7f))
    UserVIAState.ifr|=0x80;
  else
    UserVIAState.ifr&=0x7f;
  intStatus&=~(1<<userVia);
  intStatus|=((UserVIAState.ifr & 128)?(1<<userVia):0);
}; /* UpdateIFRTopBit */

/*--------------------------------------------------------------------------*/
/* Address is in the range 0-f - with the fe60 stripped out */
void UserVIAWrite(int Address, int Value) {

static int last_Value = 0xff;

/* cerr << "UserVIAWrite: Address=0x" << hex << Address << " Value=0x" << Value << dec << " at " << TotalCycles << "\n";
  DumpRegs(); */

	if (DebugEnabled) {
		char info[200];
		sprintf(info, "UsrVia: Write address %X value %02X", (int)(Address & 0xf), Value & 0xff);
		DebugDisplayTrace(DEBUG_USERVIA, true, info);
	}

  switch (Address) {
    case 0:
      UserVIAState.orb=Value & 0xff;
      if ((UserVIAState.ifr & 8) && ((UserVIAState.pcr & 0x20)==0)) {
        UserVIAState.ifr&=0xf7 ;
        UpdateIFRTopBit();
      };
		if (mBreakOutWindow) ShowOutputs(UserVIAState.orb);
			if (RTC_Enabled)
			{
				if ( ((last_Value & 0x02) == 0x02) && ((Value & 0x02) == 0x00) )		// falling clock edge
				{
					if ((Value & 0x04) == 0x04)
					{
						RTC_cmd = (RTC_cmd >> 1) | ((Value & 0x01) << 15);
						RTC_bit++;
						
						WriteLog("RTC Shift cmd : 0x%03x, bit : %d\n", RTC_cmd, RTC_bit);
						
					} else {
						
						if (RTC_bit == 11)		// Write data
						{
							RTC_cmd >>= 5;
							
							WriteLog("RTC Write cmd : 0x%03x, reg : 0x%02x, data = 0x%02x\n", RTC_cmd, (RTC_cmd & 0x0f) >> 1, RTC_cmd >> 4);
							
							RTC_ram[(RTC_cmd & 0x0f) >> 1] = RTC_cmd >> 4;
						} else {
							RTC_cmd >>= 12;
							
							time_t SysTime;
							struct tm * CurTime;
							
							time( &SysTime );
							CurTime = localtime( &SysTime );
							
							switch ((RTC_cmd & 0x0f) >> 1)
							{
								case 0 :
									RTC_data = BCD((CurTime->tm_mon)+1);
									break;
								case 1 :
									RTC_data = BCD((CurTime->tm_year % 100) - 1);
									break;
								case 2 :
									RTC_data = BCD(CurTime->tm_mday);
									break;
								case 3 :
									RTC_data = RTC_ram[3];
									break;
								case 4 :
									RTC_data = BCD(CurTime->tm_hour);
									break;
								case 5 :
									RTC_data = RTC_ram[5];
									break;
								case 6 :
									RTC_data = BCD(CurTime->tm_min);
									break;
								case 7 :
									RTC_data = RTC_ram[7];
									break;
							}
							
							WriteLog("RTC Read cmd : 0x%03x, reg : 0x%02x, data : 0x%02x\n", RTC_cmd, (RTC_cmd & 0x0f) >> 1, RTC_data);
							
						}
					}
				}
			}
				last_Value = Value;
		break;

    case 1:
      UserVIAState.ora=Value & 0xff;
      UserVIAState.ifr&=0xfc;
      UpdateIFRTopBit();
	  if (PrinterEnabled) {
		if (PrinterFileHandle != NULL)
		{
	      if (fputc(UserVIAState.ora, PrinterFileHandle) == EOF ) {
			  fprintf(stderr, "Failed to write to printer file %s\n", PrinterFileName);
		  }
		  else {
		    fflush(PrinterFileHandle);
		    SetTrigger(PRINTER_TRIGGER, PrinterTrigger);
	  	  }
		} else {		// Write to clipboard
			mainWin->CopyKey(UserVIAState.ora);
			SetTrigger(PRINTER_TRIGGER, PrinterTrigger);
		}
	  }
      break;

    case 2:
      UserVIAState.ddrb=Value & 0xff;
 	  if (RTC_Enabled && ((Value & 0x07) == 0x07)) RTC_bit = 0;
     break;

    case 3:
      UserVIAState.ddra=Value & 0xff;
      break;

    case 4:
    case 6:
      /*cerr << "UserVia Reg4 Timer1 lo Counter Write val=0x " << hex << Value << dec << " at " << TotalCycles << "\n"; */
      UserVIAState.timer1l&=0xff00;
      UserVIAState.timer1l|=(Value & 0xff);
      break;

    case 5:
      /*cerr << "UserVia Reg5 Timer1 hi Counter Write val=0x" << hex << Value << dec  << " at " << TotalCycles << "\n"; */
      UserVIAState.timer1l&=0xff;
      UserVIAState.timer1l|=(Value & 0xff)<<8;
      UserVIAState.timer1c=UserVIAState.timer1l * 2 + 1;
      UserVIAState.ifr &=0xbf; /* clear timer 1 ifr */
      /* If PB7 toggling enabled, then lower PB7 now */
      if (UserVIAState.acr & 128) {
        UserVIAState.orb&=0x7f;
        UserVIAState.irb&=0x7f;
      };
      UpdateIFRTopBit();
      UserVIAState.timer1hasshot=0; //Added by K.Lowe 24/08/03
      break;

    case 7:
      /*cerr << "UserVia Reg7 Timer1 hi latch Write val=0x" << hex << Value << dec  << " at " << TotalCycles << "\n"; */
      UserVIAState.timer1l&=0xff;
      UserVIAState.timer1l|=(Value & 0xff)<<8;
      UserVIAState.ifr &=0xbf; /* clear timer 1 ifr (this is what Model-B does) */
      UpdateIFRTopBit();
      break;

    case 8:
      /* cerr << "UserVia Reg8 Timer2 lo Counter Write val=0x" << hex << Value << dec << "\n"; */
      UserVIAState.timer2l&=0xff00;
      UserVIAState.timer2l|=(Value & 0xff);
      break;

    case 9:
      /* cerr << "UserVia Reg9 Timer2 hi Counter Write val=0x" << hex << Value << dec << "\n";
      core_dumpstate(); */
      UserVIAState.timer2l&=0xff;
      UserVIAState.timer2l|=(Value & 0xff)<<8;
      UserVIAState.timer2c=UserVIAState.timer2l * 2 + 1;
      UserVIAState.ifr &=0xdf; /* clear timer 2 ifr */
      UpdateIFRTopBit();
      UserVIAState.timer2hasshot=0; //Added by K.Lowe 24/08/03
      break;

    case 10:
      break;

    case 11:
      UserVIAState.acr=Value & 0xff;
      break;

    case 12:
      UserVIAState.pcr=Value & 0xff;
      break;

    case 13:
      UserVIAState.ifr&=~(Value & 0xff);
      UpdateIFRTopBit();
      break;

    case 14:
      // cerr << "User VIA Write ier Value=" << Value << "\n";
      if (Value & 0x80)
        UserVIAState.ier|=Value & 0xff;
      else
        UserVIAState.ier&=~(Value & 0xff);
      UserVIAState.ier&=0x7f;
      UpdateIFRTopBit();
      break;

    case 15:
      UserVIAState.ora=Value & 0xff;
      break;
  } /* Address switch */
} /* UserVIAWrite */

/*--------------------------------------------------------------------------*/
/* Address is in the range 0-f - with the fe60 stripped out */
int UserVIARead(int Address) {
  int tmp = 0xff;
  /* cerr << "UserVIARead: Address=0x" << hex << Address << dec << " at " << TotalCycles << "\n";
  DumpRegs(); */

  switch (Address) {
    case 0: /* IRB read */
      tmp=(UserVIAState.orb & UserVIAState.ddrb) | (UserVIAState.irb & (~UserVIAState.ddrb));

	  if ((UserVIAState.ifr & 0x10) && ((UserVIAState.pcr & 0x10)==0x10)) {
		  UserVIAState.ifr&=0xef;
		  UpdateIFRTopBit();
	  };
		  
	  if (RTC_Enabled)
	  {
		tmp = (tmp & 0xfe) | (RTC_data & 0x01);
		RTC_data = RTC_data >> 1;
	  }

	  if (mBreakOutWindow) ShowInputs(tmp);

	  if (AMXMouseEnabled) {
        if (AMXLRForMiddle) {
          if ((AMXButtons & AMX_LEFT_BUTTON) && (AMXButtons & AMX_RIGHT_BUTTON))
            AMXButtons = AMX_MIDDLE_BUTTON;
          else
            AMXButtons &= ~AMX_MIDDLE_BUTTON;
        }

		if (Tube186Enabled)
		{
			tmp &= 0xf8;
			tmp |= (AMXButtons ^ 7);
		}
		else
		{
			tmp &= 0x1f;
			tmp	|= (AMXButtons ^ 7) << 5;
			UserVIAState.ifr&=0xe7;
		}
		  
	  UpdateIFRTopBit();

		  /* Set up another interrupt if not at target */
        if ( (AMXTargetX != AMXCurrentX) || (AMXTargetY != AMXCurrentY) ) {
          SetTrigger(AMX_TRIGGER, AMXTrigger);
        }
        else {
          ClearTrigger(AMXTrigger);
        }
      }

      break;

    case 2:
      tmp = UserVIAState.ddrb;
      break;

    case 3:
      tmp = UserVIAState.ddra;
      break;

    case 4: /* Timer 1 lo counter */
		if (UserVIAState.timer1c < 0)
			tmp=0xff;
		else
			tmp=(UserVIAState.timer1c / 2) & 0xff;
		UserVIAState.ifr&=0xbf; /* Clear bit 6 - timer 1 */
      UpdateIFRTopBit();
      break;

    case 5: /* Timer 1 hi counter */
      tmp=(UserVIAState.timer1c>>9) & 0xff;
      break;

    case 6: /* Timer 1 lo latch */
      tmp = UserVIAState.timer1l & 0xff;
      break;

    case 7: /* Timer 1 hi latch */
      tmp = (UserVIAState.timer1l>>8) & 0xff;
      break;

    case 8: /* Timer 2 lo counter */
      if (UserVIAState.timer2c < 0) /* Adjust for dividing -ve count by 2 */
		tmp = ((UserVIAState.timer2c - 1) / 2) & 0xff;
	  else
	    tmp=(UserVIAState.timer2c / 2) & 0xff;
      UserVIAState.ifr&=0xdf; /* Clear bit 5 - timer 2 */
      UpdateIFRTopBit();
      break;

    case 9: /* Timer 2 hi counter */
      tmp=(UserVIAState.timer2c>>9) & 0xff;
      break;

    case 11:
      tmp = UserVIAState.acr;
      break;

    case 12:
      tmp = UserVIAState.pcr;
      break;

    case 13:
      UpdateIFRTopBit();
      tmp = UserVIAState.ifr;
      break;

    case 14:
      tmp = UserVIAState.ier | 0x80;
      break;

    case 1:
      UserVIAState.ifr&=0xfc;
      UpdateIFRTopBit();
    case 15:
      tmp = 255;
      break;
  } /* Address switch */

	if (DebugEnabled) {
		char info[200];
		sprintf(info, "UsrVia: Read address %X value %02X", (int)(Address & 0xf), tmp & 0xff);
		DebugDisplayTrace(DEBUG_USERVIA, true, info);
	}

  return(tmp);
} /* UserVIARead */

/*--------------------------------------------------------------------------*/
void UserVIATriggerCA1Int(void) {
  /* We should be concerned with active edges etc. */
  UserVIAState.ifr|=2; /* CA1 */
  UpdateIFRTopBit();
}; /* UserVIATriggerCA1Int */

/*--------------------------------------------------------------------------*/
void UserVIA_poll_real(void) {
	static bool t1int=false;
	
  if (UserVIAState.timer1c<-2 && !t1int) {
    t1int = true;
    if ((UserVIAState.timer1hasshot==0) || (UserVIAState.acr & 0x40)) {
      /*cerr << "UserVIA timer1c - int at " << TotalCycles << "\n"; */
      UserVIAState.ifr|=0x40; /* Timer 1 interrupt */
      UpdateIFRTopBit();
      if (UserVIAState.acr & 0x80) {
        UserVIAState.orb^=0x80; /* Toggle PB7 */
        UserVIAState.irb^=0x80; /* Toggle PB7 */
      }
   	  if ((UserVIAState.ier & 0x40) && CyclesToInt == NO_TIMER_INT_DUE) {
		  CyclesToInt = 3 + UserVIAState.timer1c;
	  }
	  UserVIAState.timer1hasshot=1;
    }
  }
	
  if (UserVIAState.timer1c<-3) {
	  UserVIAState.timer1c += (UserVIAState.timer1l * 2) + 4;
      t1int = false;
  }
	
  if (UserVIAState.timer2c<-2) {
    if (UserVIAState.timer2hasshot==0) {
     /* cerr << "UserVIA timer2c - int\n"; */
      UserVIAState.ifr|=0x20; /* Timer 2 interrupt */
      UpdateIFRTopBit();
   	  if ((UserVIAState.ier & 0x20) && CyclesToInt == NO_TIMER_INT_DUE) {
		  CyclesToInt = 3 + UserVIAState.timer2c;
	  }
	  UserVIAState.timer2hasshot=1; // Added by K.Lowe 24/08/03
    }
  } /* timer2c underflow */

  if (UserVIAState.timer2c<-3) {
	  UserVIAState.timer2c += 0x20000; // Do not reload latches for T2
  }

} /* UserVIA_poll */

void UserVIA_poll(unsigned int ncycles) {
  // Converted to a proc to allow shift register functions
  UserVIAState.timer1c-=ncycles; 
  if (!(UserVIAState.acr & 0x20))
	UserVIAState.timer2c-=ncycles; 
  if ((UserVIAState.timer1c<0) || (UserVIAState.timer2c<0)) UserVIA_poll_real();
  if (AMXMouseEnabled && AMXTrigger<=TotalCycles) AMXMouseMovement();
  if (PrinterEnabled && PrinterTrigger <= TotalCycles) PrinterPoll();
  // Do Shift register stuff
//  if (SRMode==2) {
	  // Shift IN under control of Clock 2
//	  SRCount=8-(ncycles%8);
//  }
}


/*--------------------------------------------------------------------------*/
void UserVIAReset(void) {
  VIAReset(&UserVIAState);
  ClearTrigger(AMXTrigger);
  ClearTrigger(PrinterTrigger);
} /* UserVIAReset */

int sgn(int number)
{
	if (number > 0) return 1;
	if (number < 0) return -1;
	return 0;
}

/*-------------------------------------------------------------------------*/
void AMXMouseMovement() {
static int xdir = 0;
static int ydir = 0;
static int xpulse = 0x08;
static int ypulse = 0x10;
static int lastxdir = 0;
static int lastydir = 0;
static bool first = true;

	ClearTrigger(AMXTrigger);

	/* Check if there is a outstanding interrupt */
	if (AMXMouseEnabled && (UserVIAState.ifr & 0x18) == 0)
	{
		
    	if ( (AMXTargetX != AMXCurrentX) || (AMXTargetY != AMXCurrentY) )
		{
			
            if (Tube186Enabled)
            {
				
		    	xdir = sgn(AMXTargetX - AMXCurrentX);
				
			    if (xdir != 0)
    			{
	    			UserVIAState.ifr |= 0x10;
		    		if (lastxdir == xdir) UserVIAState.irb ^= xpulse;
			    	lastxdir = xdir;
				    AMXCurrentX += xdir;
    			}
				
	    		ydir = sgn(AMXTargetY - AMXCurrentY);
				
    			if (ydir != 0)
	    		{
		    		UserVIAState.ifr |= 0x08;
					
	    			if (first)
    				{
	    				UserVIAState.irb &= ~ypulse;
		    			first = false;
				    }
					
    				if (lastydir == ydir) UserVIAState.irb ^= ypulse;
	    			lastydir = ydir;
		    		AMXCurrentY += ydir;
					
			    }
            }
            else
            {
				
		    	if (AMXTargetX != AMXCurrentX)
			    {
    				UserVIAState.ifr |= 0x10;
	    			if (AMXTargetX < AMXCurrentX)
		    		{
			    		UserVIAState.irb &= ~0x01;
				    	AMXCurrentX--;
    				}
	    			else
		    		{
			    		UserVIAState.irb |= 0x01;
				    	AMXCurrentX++;
    				}
	    		}
		    	if (AMXTargetY != AMXCurrentY)
			    {
    				UserVIAState.ifr |= 0x08;
	    			if (AMXTargetY > AMXCurrentY)
		    		{
			    		UserVIAState.irb &= ~0x04;
				    	AMXCurrentY++;
    				}
	    			else
		    		{
			    		UserVIAState.irb |= 0x04;
    					AMXCurrentY--;
	    			}
		    	}
            }
		    UpdateIFRTopBit();
        }
	}
	
}

/*-------------------------------------------------------------------------*/
void PrinterEnable(char *FileName) {
	/* Close file if already open */
	if (PrinterFileHandle != NULL)
	{
		fclose(PrinterFileHandle);
		PrinterFileHandle = NULL;
	}

    if (strcmp(FileName, "CLIPBOARD") == 0)
	{
	}
	else
	{
		strcpy(PrinterFileName, FileName);
		PrinterFileHandle = fopen(FileName, "wb");
		if (PrinterFileHandle == NULL)
		{
			fprintf(stderr, "Failed to open printer %s\n", PrinterFileName);
			return;
		}
	}
	PrinterEnabled = 1;
	SetTrigger(PRINTER_TRIGGER, PrinterTrigger);
}

/*-------------------------------------------------------------------------*/
void PrinterDisable() {
	if (PrinterFileHandle != NULL)
	{
		fclose(PrinterFileHandle);
		PrinterFileHandle = NULL;
	}

	PrinterEnabled = 0;
	ClearTrigger(PrinterTrigger);
}
/*-------------------------------------------------------------------------*/
void PrinterPoll() {
	ClearTrigger(PrinterTrigger);
	UserVIATriggerCA1Int();

	/* The CA1 interrupt is not always picked up,
		set up a trigger just in case. */
	SetTrigger(100000, PrinterTrigger);
}

/*--------------------------------------------------------------------------*/
void uservia_dumpstate(void) {
  fprintf(stderr, "Uservia:\n");
  via_dumpstate(&UserVIAState);
}; /* uservia_dumpstate */

void DebugUserViaState()
{
	DebugViaState("UsrVia", &UserVIAState);
}

//*******************************************************************

OSStatus BreakOutWindowCommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
	int bit;
    HICommand command; 
    OSStatus err = noErr;
    err = GetEventParameter(event, kEventParamDirectObject,
							typeHICommand, NULL, sizeof(HICommand), NULL, &command);
    require_noerr (err, CantGetParameter);
	
	err = noErr;

//	fprintf(stderr, "commandID = 0x%08x\n", command.commandID);
	
	SetBitKey(command.commandID);
	
	if (BitKey == -1)
	{

		switch (command.commandID)
    {
        case 'uib7':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B7 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x80) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x80; else UserVIAState.irb |= 0x80;
			}
			break;
        case 'uib6':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B6 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x40) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x40; else UserVIAState.irb |= 0x40;
			}
			break;
        case 'uib5':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B5 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x20) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x20; else UserVIAState.irb |= 0x20;
			}
			break;
        case 'uib4':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B4 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x10) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x10; else UserVIAState.irb |= 0x10;
			}
			break;
        case 'uib3':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B3 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x08) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x08; else UserVIAState.irb |= 0x08;
			}
			break;
        case 'uib2':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B2 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x04) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x04; else UserVIAState.irb |= 0x04;
			}
			break;
        case 'uib1':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B1 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x02) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x02; else UserVIAState.irb |= 0x02;
			}
			break;
        case 'uib0':
			bit = GetValue(command.commandID);
//          fprintf(stderr, "User Input B0 - bit = %d\n", bit);
			if ((UserVIAState.ddrb & 0x01) == 0x00)
			{
				if (bit == 1) UserVIAState.irb &= ~0x01; else UserVIAState.irb |= 0x01;
			}
			break;
			
        default:
            err = eventNotHandledErr;
            break;
    }

	}
	else
		{
			ShowBitKey(BitKey, command.commandID);
		}

CantGetParameter:
		return err;
}

static OSStatus BreakOutWindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    OSStatus err = noErr;
	char charCode;
	int keycode;

    switch (GetEventKind(event))
    {

		case kEventRawKeyDown:
			GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
			GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(int), NULL, &keycode);
//			fprintf(stderr, "Key pressed: code = %d, '%c'\n", keycode, charCode);
			if (LastBitButton != 0)
			{
				BitKeys[BitKey] = keycode;
				ShowBitKey(BitKey, LastBitButton);
			}
			break;
			
		case kEventWindowClosed: 
			mBreakOutWindow = NULL;
			mainWin->SetMenuCommandIDCheck('upbo', false);
            break;

        default:
            err = eventNotHandledErr;
            break;
    }
    
    return err;
}

void BreakOutOpenDialog()
{

	IBNibRef 		nibRef;
	EventTypeSpec BreakOutCommands[] = {
	{ kEventClassCommand, kEventCommandProcess }
	};
		
	EventTypeSpec BreakOutEvents[] = {
	{ kEventClassWindow, kEventWindowClosed },
	{ kEventClassKeyboard, kEventRawKeyDown}
	};

	if (mBreakOutWindow == NULL)
	{
		// Create a Nib reference passing the name of the nib file (without the .nib extension)
		// CreateNibReference only searches into the application bundle.
		CreateNibReference(CFSTR("main"), &nibRef);
		CreateWindowFromNib(nibRef, CFSTR("Window3"), &mBreakOutWindow);
		DisposeNibReference(nibRef);
		ShowWindow(mBreakOutWindow);
		
		InstallWindowEventHandler(mBreakOutWindow, 
							  NewEventHandlerUPP (BreakOutWindowCommandHandler), 
							  GetEventTypeCount(BreakOutCommands), BreakOutCommands, 
							  mBreakOutWindow, NULL);
		
		InstallWindowEventHandler (mBreakOutWindow, 
								   NewEventHandlerUPP (BreakOutWindowEventHandler), 
								   GetEventTypeCount(BreakOutEvents), BreakOutEvents, 
								   mBreakOutWindow, NULL);
		
		ShowInputs( (UserVIAState.orb & UserVIAState.ddrb) | (UserVIAState.irb & (~UserVIAState.ddrb)) );
		ShowOutputs(UserVIAState.orb);
		
		ShowBitKey(0, '0   ');
		ShowBitKey(1, '1   ');
		ShowBitKey(2, '2   ');
		ShowBitKey(3, '3   ');
		ShowBitKey(4, '4   ');
		ShowBitKey(5, '5   ');
		ShowBitKey(6, '6   ');
		ShowBitKey(7, '7   ');

	
	}
}

void BreakOutCloseDialog()
{
	if (mBreakOutWindow)
	{
		HideWindow(mBreakOutWindow);
		DisposeWindow(mBreakOutWindow);
	}
	mBreakOutWindow = NULL;
	mainWin->SetMenuCommandIDCheck('upbo', false);
}

int GetValue(OSType box)

{
	ControlID dbControlID;
	ControlRef dbControl;
	int ret;
	
	dbControlID.signature = box;
	dbControlID.id = 0;
	GetControlByID (mBreakOutWindow, &dbControlID, &dbControl);
	ret = GetControlValue(dbControl);
	return ret;
}

void SetValue(OSType box, int State)
{
	ControlID dbControlID;
	ControlRef dbControl;
	
	dbControlID.signature = box;
	dbControlID.id = 0;
	GetControlByID (mBreakOutWindow, &dbControlID, &dbControl);
	SetControlValue(dbControl, State);
}

void ShowOutputs(unsigned char data)
{
static unsigned char last_data = 0;
unsigned char changed_bits;
	
	if (mBreakOutWindow)
	{
		if (data != last_data)
		{
			changed_bits = data ^ last_data;
			if (changed_bits & 0x80) { if ((UserVIAState.ddrb & 0x80) == 0x80) SetValue('uob7', (data & 0x80) != 0); else SetValue('uob7', 0); }
			if (changed_bits & 0x40) { if ((UserVIAState.ddrb & 0x40) == 0x40) SetValue('uob6', (data & 0x40) != 0); else SetValue('uob6', 0); }
			if (changed_bits & 0x20) { if ((UserVIAState.ddrb & 0x20) == 0x20) SetValue('uob5', (data & 0x20) != 0); else SetValue('uob5', 0); }
			if (changed_bits & 0x10) { if ((UserVIAState.ddrb & 0x10) == 0x10) SetValue('uob4', (data & 0x10) != 0); else SetValue('uob4', 0); }
			if (changed_bits & 0x08) { if ((UserVIAState.ddrb & 0x08) == 0x08) SetValue('uob3', (data & 0x08) != 0); else SetValue('uob3', 0); }
			if (changed_bits & 0x04) { if ((UserVIAState.ddrb & 0x04) == 0x04) SetValue('uob2', (data & 0x04) != 0); else SetValue('uob2', 0); }
			if (changed_bits & 0x02) { if ((UserVIAState.ddrb & 0x02) == 0x02) SetValue('uob1', (data & 0x02) != 0); else SetValue('uob1', 0); }
			if (changed_bits & 0x01) { if ((UserVIAState.ddrb & 0x01) == 0x01) SetValue('uob0', (data & 0x01) != 0); else SetValue('uob0', 0); }
			last_data = data;
		}
	}
}

void ShowInputs(unsigned char data)
{
static unsigned char last_data = 0;
unsigned char changed_bits;

	if (mBreakOutWindow)
	{
		if (data != last_data)
		{
			changed_bits = data ^ last_data;
			if (changed_bits & 0x80) { if ((UserVIAState.ddrb & 0x80) == 0x00) SetValue('uib7', (data & 0x80) == 0); else SetValue('uib7', 0); }
			if (changed_bits & 0x40) { if ((UserVIAState.ddrb & 0x40) == 0x00) SetValue('uib6', (data & 0x40) == 0); else SetValue('uib6', 0); }
			if (changed_bits & 0x20) { if ((UserVIAState.ddrb & 0x20) == 0x00) SetValue('uib5', (data & 0x20) == 0); else SetValue('uib5', 0); }
			if (changed_bits & 0x10) { if ((UserVIAState.ddrb & 0x10) == 0x00) SetValue('uib4', (data & 0x10) == 0); else SetValue('uib4', 0); }
			if (changed_bits & 0x08) { if ((UserVIAState.ddrb & 0x08) == 0x00) SetValue('uib3', (data & 0x08) == 0); else SetValue('uib3', 0); }
			if (changed_bits & 0x04) { if ((UserVIAState.ddrb & 0x04) == 0x00) SetValue('uib2', (data & 0x04) == 0); else SetValue('uib2', 0); }
			if (changed_bits & 0x02) { if ((UserVIAState.ddrb & 0x02) == 0x00) SetValue('uib1', (data & 0x02) == 0); else SetValue('uib1', 0); }
			if (changed_bits & 0x01) { if ((UserVIAState.ddrb & 0x01) == 0x00) SetValue('uib0', (data & 0x01) == 0); else SetValue('uib0', 0); }
			last_data = data;
		}
	}
}

void ShowBitKey(int key, int ctrlID)
{
char Keys[256];
CFStringRef pTitle;

	strcpy(Keys, BitKeyName(BitKeys[key]));

//	fprintf(stderr, "Setting button text to '%s'\n", Keys);

	ControlID dbKeyID;
	ControlRef dbControl;
	
	dbKeyID.signature = ctrlID;
	dbKeyID.id = 0;
	
	GetControlByID (mBreakOutWindow, &dbKeyID, &dbControl);

	pTitle = CFStringCreateWithCString (kCFAllocatorDefault, Keys, kCFStringEncodingASCII);

    SetControlTitleWithCFString(dbControl, pTitle);
	
	CFRelease(pTitle);

	if ( (LastBitButton != 0) && (LastBitButton != ctrlID) )
	{

		ControlID dbKeyID;
		
		dbKeyID.signature = LastBitButton;
		dbKeyID.id = 0;
		
		GetControlByID (mBreakOutWindow, &dbKeyID, &dbControl);

		SetControlValue(dbControl, 0);
	}

	LastBitButton = ctrlID;

}

const char *BitKeyName( int Key )
{
	switch( Key )
	{
	case   0: return "A";
	case   1: return "S";
	case   2: return "D";
	case   3: return "F";
	case   4: return "H";
	case   5: return "G";
	case   6: return "Z";
	case   7: return "X";
	case   8: return "C";
	case   9: return "V";
	case  10: return "Left of One";
	case  11: return "B";
	case  12: return "Q";
	case  13: return "W";
	case  14: return "E";
	case  15: return "R";
	case  16: return "Y";
	case  17: return "T";
	case  18: return "1";
	case  19: return "2";
	case  20: return "3";
	case  21: return "4";
	case  22: return "6";
	case  23: return "5";
	case  24: return "=";
	case  25: return "9";
	case  26: return "7";
	case  27: return "-";
	case  28: return "8";
	case  29: return "0";
	case  30: return "]";
	case  31: return "O";
	case  32: return "U";
	case  33: return "[";
	case  34: return "I";
	case  35: return "P";
	case  36: return "Return";
	case  37: return "L";
	case  38: return "J";
	case  39: return "'";
	case  40: return "K";
	case  41: return ";";
	case  42: return "\\";
	case  43: return ",";
	case  44: return "/";
	case  45: return "N";
	case  46: return "M";
	case  47: return ".";
	case  48: return "Tab";
	case  49: return "Space";
	case  50: return "`";
	case  51: return "Backspace";
	case  52: return "Unknown";
	case  53: return "Escape";

	case  96: return "F5";
	case  97: return "F6";
	case  98: return "F7";
	case  99: return "F3";
	case 100: return "F8";
	case 101: return "F9";
	case 103: return "F11";
	case 105: return "F13";
	case 106: return "F16";
	case 107: return "F14";
	case 109: return "F10";
	case 111: return "F12";
	case 113: return "F15";
	case 114: return "Help";
	case 115: return "Home";
	case 116: return "PgUp";
	case 117: return "Delete";
	case 118: return "F4";
	case 119: return "End";
	case 120: return "F2";
	case 121: return "PgDn";
	case 122: return "F1";
	case 123: return "Left";
	case 124: return "Right";
	case 125: return "Down";
	case 126: return "Up";
		
	default:
		return " ";
	}

}

/****************************************************************************/

void SetBitKey( int ctrlID )
{
	switch( ctrlID )
	{
	// Character keys.
	case '0   ': BitKey = 0; break;
	case '1   ': BitKey = 1; break;
	case '2   ': BitKey = 2; break;
	case '3   ': BitKey = 3; break;
	case '4   ': BitKey = 4; break;
	case '5   ': BitKey = 5; break;
	case '6   ': BitKey = 6; break;
	case '7   ': BitKey = 7; break;
	
	default:
		BitKey = -1;
	}

//	fprintf(stderr, "Key %08x, BitKey = %d\n", ctrlID, BitKey);

}
