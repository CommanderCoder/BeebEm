/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 1994  David Alan Gilbert
Copyright (C) 1997  Mike Wyatt

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with this program; if not, write to the Free
Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA  02110-1301, USA.
****************************************************************/

/* 04/12/1994 David Alan Gilbert: 8271 disc emulation  */
/* 30/08/1997 Mike Wyatt: Added disc write and format support */
/* 27/12/2011 J.G.Harston: Double-sided SSD supported */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "6502core.h"
#include "disc8271.h"

#include "main.h"
#include "beebmem.h"
#include "disc1770.h"
#include "uefstate.h"
#include "beebsound.h"
#include "sysvia.h"
#include "tube.h"
// #include "log.h"

extern Tube TubeType;

#define ENABLE_LOG 0

// 8271 Status register
const unsigned char STATUS_REG_COMMAND_BUSY       = 0x80;
const unsigned char STATUS_REG_COMMAND_FULL       = 0x40;
const unsigned char STATUS_REG_PARAMETER_FULL     = 0x20;
const unsigned char STATUS_REG_RESULT_FULL        = 0x10;
const unsigned char STATUS_REG_INTERRUPT_REQUEST  = 0x08;
const unsigned char STATUS_REG_NON_DMA_MODE       = 0x04;

// 8271 Result register
const unsigned char RESULT_REG_SUCCESS            = 0x00;
const unsigned char RESULT_REG_SCAN_NOT_MET       = 0x00;
const unsigned char RESULT_REG_SCAN_MET_EQUAL     = 0x02;
const unsigned char RESULT_REG_SCAN_MET_NOT_EQUAL = 0x04;
const unsigned char RESULT_REG_CLOCK_ERROR        = 0x08;
const unsigned char RESULT_REG_LATE_DMA           = 0x0A;
const unsigned char RESULT_REG_ID_CRC_ERROR       = 0x0C;
const unsigned char RESULT_REG_DATA_CRC_ERROR     = 0x0E;
const unsigned char RESULT_REG_DRIVE_NOT_READY    = 0x10;
const unsigned char RESULT_REG_WRITE_PROTECT      = 0x12;
const unsigned char RESULT_REG_TRACK_0_NOT_FOUND  = 0x14;
const unsigned char RESULT_REG_WRITE_FAULT        = 0x16;
const unsigned char RESULT_REG_SECTOR_NOT_FOUND   = 0x18;
const unsigned char RESULT_REG_DRIVE_NOT_PRESENT  = 0x1E; // Undocumented, see http://beebwiki.mdfs.net/OSWORD_%267F
const unsigned char RESULT_REG_DELETED_DATA_FOUND = 0x20;

// 8271 special registers
const unsigned char SPECIAL_REG_SCAN_SECTOR_NUMBER        = 0x06;
const unsigned char SPECIAL_REG_SCAN_COUNT_MSB            = 0x14;
const unsigned char SPECIAL_REG_SCAN_COUNT_LSB            = 0x13;
const unsigned char SPECIAL_REG_SURFACE_0_CURRENT_TRACK   = 0x12;
const unsigned char SPECIAL_REG_SURFACE_1_CURRENT_TRACK   = 0x1A;
const unsigned char SPECIAL_REG_MODE_REGISTER             = 0x17;
const unsigned char SPECIAL_REG_DRIVE_CONTROL_OUTPUT_PORT = 0x23;
const unsigned char SPECIAL_REG_DRIVE_CONTROL_INPUT_PORT  = 0x22;
const unsigned char SPECIAL_REG_SURFACE_0_BAD_TRACK_1     = 0x10;
const unsigned char SPECIAL_REG_SURFACE_0_BAD_TRACK_2     = 0x11;
const unsigned char SPECIAL_REG_SURFACE_1_BAD_TRACK_1     = 0x18;
const unsigned char SPECIAL_REG_SURFACE_1_BAD_TRACK_2     = 0x19;

bool Disc8271Enabled = true;
int Disc8271Trigger; /* Cycle based time Disc8271Trigger */
static unsigned char ResultReg;
static unsigned char StatusReg;
static unsigned char DataReg;
static unsigned char Internal_Scan_SectorNum;
static unsigned int Internal_Scan_Count; /* Read as two bytes */
static unsigned char Internal_ModeReg;
static unsigned char Internal_CurrentTrack[2]; /* 0/1 for surface number */
static unsigned char Internal_DriveControlOutputPort;
static unsigned char Internal_DriveControlInputPort;
static unsigned char Internal_BadTracks[2][2]; /* 1st subscript is surface 0/1 and second subscript is badtrack 0/1 */

// State set by the Specify (initialisation) command
// See Intel 8271 data sheet, page 15, ADUG page 39-40
static int StepRate; // In 2ms steps
static int HeadSettlingTime; // In 2ms steps
static int IndexCountBeforeHeadUnload; // Number of revolutions (0 to 14), or 15 to keep loaded
static int HeadLoadTime; // In 8ms steps

static int DriveHeadPosition[2]={0};
static bool DriveHeadLoaded=false;
static bool DriveHeadUnloadPending=false;

static int ThisCommand;
static int NParamsInThisCommand;
static int PresentParam; /* From 0 */
static unsigned char Params[16]; /* Wildly more than we need */

// These bools indicate which drives the last command selected.
// They also act as "drive ready" bits which are reset when the motor stops.
static bool Selects[2]; /* Drive selects */
static bool Writeable[2]={false,false}; /* True if the drives are writeable */

static bool FirstWriteInt; // Indicates the start of a write operation 

static unsigned char NextInterruptIsErr; // none 0 causes error and drops this value into result reg

#define TRACKSPERDRIVE 80

/* Note Head select is done from bit 5 of the drive output register */
#define CURRENTHEAD ((Internal_DriveControlOutputPort>>5) & 1)

/* Note: reads/writes one byte every 80us */
#define TIMEBETWEENBYTES (160)

typedef struct {

  struct {       
    unsigned int CylinderNum:7;
    unsigned int RecordNum:5;
    unsigned int HeadNum:1;
    unsigned int PhysRecLength;
  } IDField;

  bool Deleted; // If non-zero the sector is deleted
  unsigned char *Data;
} SectorType;

typedef struct {
  int LogicalSectors; /* Number of sectors stated in format command */
  int NSectors; /* i.e. the number of records we have - not anything physical */
  SectorType *Sectors;
  int Gap1Size,Gap3Size,Gap5Size; /* From format command */
} TrackType;

/* All data on the disc - first param is drive number, then head. then physical track id */
TrackType DiscStore[2][2][TRACKSPERDRIVE];

/* File names of loaded disc images */
static char FileNames[2][256];

/* Number of sides of loaded disc images */
static int NumHeads[2];

static bool SaveTrackImage(int DriveNum, int HeadNum, int TrackNum);
static void DriveHeadScheduleUnload(void);

typedef void (*CommandFunc)(void);

#define UPDATENMISTATUS                           \
  if (StatusReg & STATUS_REG_INTERRUPT_REQUEST) { \
    NMIStatus |= 1 << nmi_floppy;                 \
  }                                               \
  else {                                          \
    NMIStatus &= ~(1 << nmi_floppy);              \
  }

/*--------------------------------------------------------------------------*/
struct {
  int TrackAddr;
  int CurrentSector;
  int SectorLength; /* In bytes */
  int SectorsToGo;

  SectorType *CurrentSectorPtr;
  TrackType *CurrentTrackPtr;

  int ByteWithinSector; /* Next byte in sector or ID field */
} CommandStatusType;

static CommandStatusType CommandStatus;

/*--------------------------------------------------------------------------*/
typedef struct  {
  unsigned char CommandNum;
  unsigned char Mask; /* Mask command with this before comparing with CommandNum - allows drive ID to be removed */
  int NParams; /* Number of parameters to follow */
  CommandFunc ToCall; /* Called after all paameters have arrived */
  CommandFunc IntHandler; /* Called when interrupt requested by command is about to happen */
  const char *Ident; /* Mainly for debugging */
} PrimaryCommandLookupType; 

/*--------------------------------------------------------------------------*/
/* For appropriate commands checks the select bits in the command code and  */
/* selects the appropriate drive.                                           */
static void DoSelects(void) {
  Selects[0]=(ThisCommand & 0x40)!=0;
  Selects[1]=(ThisCommand & 0x80)!=0;
  Internal_DriveControlOutputPort&=0x3f;
  if (Selects[0]) Internal_DriveControlOutputPort|=0x40;
  if (Selects[1]) Internal_DriveControlOutputPort|=0x80;	
};

/*--------------------------------------------------------------------------*/
static void NotImp(const char *NotImpCom) {
#ifdef WIN32
  char errstr[200];
  sprintf(errstr,"Disc operation '%s' not supported", NotImpCom);
  MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
#else
  fprintf(stderr, "%s has not been implemented in disc8271 - sorry\n", NotImpCom);
  exit(0);
#endif
}; 

/*--------------------------------------------------------------------------*/
/* Load the head - ignore for the moment                                    */
static void DoLoadHead(void) {
} 

/*--------------------------------------------------------------------------*/
/* Initialise our disc structures                                           */
static void InitDiscStore(void) {
  int head,track,drive;
  TrackType blank={0,0,NULL,0,0,0};

  for(drive=0;drive<2;drive++)
    for(head=0;head<2;head++)
      for(track=0;track<TRACKSPERDRIVE;track++)
        DiscStore[drive][head][track]=blank;
}

/*--------------------------------------------------------------------------*/
/* Given a logical track number accounts for bad tracks                     */
static int SkipBadTracks(int Unit, int trackin) {
  int offset=0;
  if (TubeType != Tube::TorchZ80)	// If running under Torch Z80, ignore bad tracks
  {
	if (Internal_BadTracks[Unit][0]<=trackin) offset++;
    if (Internal_BadTracks[Unit][1]<=trackin) offset++;
  }  
  return(trackin+offset);
}

/*--------------------------------------------------------------------------*/

static int GetSelectedDrive() {
  if (Selects[0]) {
    return 0;
  }

  if (Selects[1]) {
    return 1;
  }

  return -1;
}

/*--------------------------------------------------------------------------*/
/* Returns a pointer to the data structure for a particular track.  You     */
/* pass the logical track number, it takes into account bad tracks and the  */
/* drive select and head select etc.  It always returns a valid ptr - if    */
/* there aren't that many tracks then it uses the last one.                 */
/* The one exception!!!! is that if no drives are selected it returns NULL  */
static TrackType *GetTrackPtr(int LogicalTrackID) {
  int Drive = GetSelectedDrive();
  
  if (Drive < 0) {
      return nullptr;
  }

  LogicalTrackID=SkipBadTracks(Drive, LogicalTrackID);

  if (LogicalTrackID>=TRACKSPERDRIVE) LogicalTrackID=TRACKSPERDRIVE-1;

  return &DiscStore[Drive][CURRENTHEAD][LogicalTrackID];
}

/*--------------------------------------------------------------------------*/
/* Returns a pointer to the data structure for a particular sector. Returns */
/* NULL for Sector not found. Doesn't check cylinder/head ID                */
static SectorType *GetSectorPtr(const TrackType *Track, int LogicalSectorID, bool FindDeleted) {
  int CurrentSector;

  if (Track->Sectors==NULL) return(NULL);

  for(CurrentSector=0;CurrentSector<Track->NSectors;CurrentSector++)
    if ((Track->Sectors[CurrentSector].IDField.RecordNum==LogicalSectorID) && ((!Track->Sectors[CurrentSector].Deleted) || (!FindDeleted)))
      return(&(Track->Sectors[CurrentSector]));

  return(NULL);
}

/*--------------------------------------------------------------------------*/
/* Returns true if the drive signified by the current selects is ready      */
static bool CheckReady(void) {
	return Selects[0] || Selects[1];
}

/*--------------------------------------------------------------------------*/

// Cause an error - pass err num                                            */

static void DoErr(unsigned char ErrNum) {
  SetTrigger(50, Disc8271Trigger); // Give it a bit of time
  NextInterruptIsErr = ErrNum;
  StatusReg = STATUS_REG_COMMAND_BUSY; // Command is busy - come back when I have an interrupt
  UPDATENMISTATUS;
}

/*--------------------------------------------------------------------------*/

/* Checks a few things in the sector - returns true if OK                   */

static bool ValidateSector(const SectorType *Sector, int Track, int SecLength) {
  if (Sector->IDField.CylinderNum != Track) {
      return false;
  }

  if (Sector->IDField.PhysRecLength != SecLength) {
      return false;
  }
 
  return true;
}
/*--------------------------------------------------------------------------*/
static void DoVarLength_ScanDataCommand(void) {
  DoSelects();
  NotImp("DoVarLength_ScanDataCommand");
}

/*--------------------------------------------------------------------------*/
static void DoVarLength_ScanDataAndDeldCommand(void) {
  DoSelects();
  NotImp("DoVarLength_ScanDataAndDeldCommand");
}

/*--------------------------------------------------------------------------*/
static void Do128ByteSR_WriteDataCommand(void) {
  DoSelects();
  NotImp("Do128ByteSR_WriteDataCommand");
}

/*--------------------------------------------------------------------------*/
static void DoVarLength_WriteDataCommand(void) {
  DoSelects();
  DoLoadHead();

  const int Drive = GetSelectedDrive();


  if (Drive < 0) {
    DoErr(RESULT_REG_DRIVE_NOT_READY);
    return;
  }

  if (!Writeable[Drive]) {
    DoErr(RESULT_REG_WRITE_PROTECT);
    return;
  }

  Internal_CurrentTrack[Drive]=Params[0];
  CommandStatus.CurrentTrackPtr=GetTrackPtr(Params[0]);
  if (CommandStatus.CurrentTrackPtr==NULL) {
    DoErr(RESULT_REG_DRIVE_NOT_READY);
    return;
  }

  CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr, Params[1], false);
  if (CommandStatus.CurrentSectorPtr==NULL) {
    DoErr(RESULT_REG_DRIVE_NOT_PRESENT); // Sector not found
    return;
  }

  CommandStatus.TrackAddr=Params[0];
  CommandStatus.CurrentSector=Params[1];
  CommandStatus.SectorsToGo=Params[2] & 31;
  CommandStatus.SectorLength=1<<(7+((Params[2] >> 5) & 7));

  if (ValidateSector(CommandStatus.CurrentSectorPtr,CommandStatus.TrackAddr,CommandStatus.SectorLength)) {
    CommandStatus.ByteWithinSector=0;
    SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger);
    StatusReg = STATUS_REG_COMMAND_BUSY;
    UPDATENMISTATUS;
    CommandStatus.ByteWithinSector=0;
    FirstWriteInt = true;
  } else {
    DoErr(RESULT_REG_DRIVE_NOT_PRESENT); // Sector not found
  }
}

/*--------------------------------------------------------------------------*/
static void WriteInterrupt(void) {
  bool LastByte = false;

  if (CommandStatus.SectorsToGo < 0) {
    StatusReg = STATUS_REG_RESULT_FULL | STATUS_REG_INTERRUPT_REQUEST;
    UPDATENMISTATUS;
    return;
  }

  if (!FirstWriteInt)
    CommandStatus.CurrentSectorPtr->Data[CommandStatus.ByteWithinSector++]=DataReg;
  else
    FirstWriteInt = false;

  ResultReg=0;
  if (CommandStatus.ByteWithinSector>=CommandStatus.SectorLength) {
    CommandStatus.ByteWithinSector=0;
    if (--CommandStatus.SectorsToGo) {
      CommandStatus.CurrentSector++;
      CommandStatus.CurrentSectorPtr = GetSectorPtr(CommandStatus.CurrentTrackPtr,
                                                    CommandStatus.CurrentSector,
                                                    false);
      if (CommandStatus.CurrentSectorPtr==NULL) {
        DoErr(RESULT_REG_DRIVE_NOT_PRESENT); // Sector not found
        return;
      }
    } else {
      /* Last sector done, write the track back to disc */
		if (SaveTrackImage(Selects[0] ? 0 : 1, CURRENTHEAD, CommandStatus.TrackAddr)) {
      StatusReg = STATUS_REG_RESULT_FULL;
      UPDATENMISTATUS;
      LastByte = true;
      CommandStatus.SectorsToGo=-1; /* To let us bail out */
      SetTrigger(0,Disc8271Trigger); /* To pick up result */
		}
		else {
			DoErr(RESULT_REG_WRITE_PROTECT);
		}
    }
  }
  
  if (!LastByte) {
    StatusReg = STATUS_REG_COMMAND_BUSY |
                STATUS_REG_INTERRUPT_REQUEST |
                STATUS_REG_NON_DMA_MODE;
    UPDATENMISTATUS;
    SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger);
  }
}

/*--------------------------------------------------------------------------*/
static void Do128ByteSR_WriteDeletedDataCommand(void) {
  DoSelects();
  NotImp("Do128ByteSR_WriteDeletedDataCommand");
}

/*--------------------------------------------------------------------------*/
static void DoVarLength_WriteDeletedDataCommand(void) {
  DoSelects();
  NotImp("DoVarLength_WriteDeletedDataCommand");
}

/*--------------------------------------------------------------------------*/
static void Do128ByteSR_ReadDataCommand(void) {
  DoSelects();
  NotImp("Do128ByteSR_ReadDataCommand");
}

/*--------------------------------------------------------------------------*/
static void DoVarLength_ReadDataCommand(void) {
  int Drive=-1;

  DoSelects();
  DoLoadHead();

  if (!CheckReady()) {
    DoErr(0x10); /* Drive not ready */
    return;
  };

  if (Selects[0]) Drive=0;
  if (Selects[1]) Drive=1;

  Internal_CurrentTrack[Drive]=Params[0];
  CommandStatus.CurrentTrackPtr=GetTrackPtr(Params[0]);
  if (CommandStatus.CurrentTrackPtr==NULL) {
    DoErr(0x10);
    return;
  };

  CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,Params[1],0);
  if (CommandStatus.CurrentSectorPtr==NULL) {
    DoErr(0x1e); /* Sector not found */
    return;
  };

  CommandStatus.TrackAddr=Params[0];
  CommandStatus.CurrentSector=Params[1];
  CommandStatus.SectorsToGo=Params[2] & 31;
  CommandStatus.SectorLength=1<<(7+((Params[2] >> 5) & 7));

  if (ValidateSector(CommandStatus.CurrentSectorPtr,CommandStatus.TrackAddr,CommandStatus.SectorLength)) {
    CommandStatus.ByteWithinSector=0;
    SetMotor(Drive,true);
    SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger);
    StatusReg=0x80; /* Command busy */
    UPDATENMISTATUS;
    CommandStatus.ByteWithinSector=0;
  } else {
    DoErr(0x1e); /* Sector not found */
  };
};

/*--------------------------------------------------------------------------*/
static void ReadInterrupt(void) {
  int LastByte=0;

  if (CommandStatus.SectorsToGo<0) {
    StatusReg=0x18; /* Result and interrupt */
    if (Selects[0])
      SetMotor(0,false);
    if (Selects[1])
      SetMotor(1,false);
    UPDATENMISTATUS;
    return;
  };

  DataReg=CommandStatus.CurrentSectorPtr->Data[CommandStatus.ByteWithinSector++];
  /*cerr << "ReadInterrupt called - DataReg=0x" << hex << int(DataReg) << dec << "ByteWithinSector=" << CommandStatus.ByteWithinSector << "\n"; */

  /* DumpAfterEach=1; */
  ResultReg=0;
  if (CommandStatus.ByteWithinSector>=CommandStatus.SectorLength) {
    CommandStatus.ByteWithinSector=0;
    /* I don't know if this can cause the thing to step - I presume not for the moment */
    if (--CommandStatus.SectorsToGo) {
      CommandStatus.CurrentSector++;
      if (CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,CommandStatus.CurrentSector,0),
          CommandStatus.CurrentSectorPtr==NULL) {
        DoErr(0x1e); /* Sector not found */
        return;
      }/* else cerr << "all ptr for sector " << CommandStatus.CurrentSector << "\n"*/;
    } else {
      /* Last sector done */
      StatusReg=0x9c;
      UPDATENMISTATUS;
      LastByte=1;
      CommandStatus.SectorsToGo=-1; /* To let us bail out */
      SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger); /* To pick up result */
    };
  };
  
  if (!LastByte) {
    StatusReg=0x8c; /* Command busy, */
    UPDATENMISTATUS;
    SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger);
  };
}; /* ReadInterrupt */

/*--------------------------------------------------------------------------*/
static void Do128ByteSR_ReadDataAndDeldCommand(void) {
  DoSelects();
  NotImp("Do128ByteSR_ReadDataAndDeldCommand");
};
/*--------------------------------------------------------------------------*/
static void DoVarLength_ReadDataAndDeldCommand(void) {
  /* Use normal read command for now - deleted data not supported */
  DoVarLength_ReadDataCommand();
};
/*--------------------------------------------------------------------------*/
static void DoReadIDCommand(void) {
  int Drive=-1;

  DoSelects();
  DoLoadHead();

  if (!CheckReady()) {
    DoErr(0x10); /* Drive not ready */
    return;
  };

  if (Selects[0]) Drive=0;
  if (Selects[1]) Drive=1;

  Internal_CurrentTrack[Drive]=Params[0];
  CommandStatus.CurrentTrackPtr=GetTrackPtr(Params[0]);
  if (CommandStatus.CurrentTrackPtr==NULL) {
    DoErr(0x10);
    return;
  };

  CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,0,0);
  if (CommandStatus.CurrentSectorPtr==NULL) {
    DoErr(0x1e); /* Sector not found */
    return;
  };

  CommandStatus.TrackAddr=Params[0];
  CommandStatus.CurrentSector=0;
  CommandStatus.SectorsToGo=Params[2];

  CommandStatus.ByteWithinSector=0;
  SetMotor(Drive,true);
  SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger);
  StatusReg=0x80; /* Command busy */
  UPDATENMISTATUS;
  CommandStatus.ByteWithinSector=0;
};

/*--------------------------------------------------------------------------*/
static void ReadIDInterrupt(void) {
  int LastByte=0;

  if (CommandStatus.SectorsToGo<0) {
    StatusReg=0x18; /* Result and interrupt */
    UPDATENMISTATUS;
	if (Selects[0])
      SetMotor(0,false);
	if (Selects[1])
      SetMotor(1,false);
    return;
  };

  if (CommandStatus.ByteWithinSector==0)
    DataReg=CommandStatus.CurrentSectorPtr->IDField.CylinderNum;
  else if (CommandStatus.ByteWithinSector==1)
    DataReg=CommandStatus.CurrentSectorPtr->IDField.HeadNum;
  else if (CommandStatus.ByteWithinSector==2)
    DataReg=CommandStatus.CurrentSectorPtr->IDField.RecordNum;
  else
    DataReg=1; /* 1=256 byte sector length */

  CommandStatus.ByteWithinSector++;

  ResultReg=0;
  if (CommandStatus.ByteWithinSector>=4) {
    CommandStatus.ByteWithinSector=0;
    if (--CommandStatus.SectorsToGo) {
      CommandStatus.CurrentSector++;
      CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,
                                                      CommandStatus.CurrentSector,0);
      if (CommandStatus.CurrentSectorPtr==NULL) {
        DoErr(0x1e); /* Sector not found */
        return;
      }
    } else {
      /* Last sector done */
      StatusReg=0x9c;
      UPDATENMISTATUS;
      LastByte=1;
      CommandStatus.SectorsToGo=-1; /* To let us bail out */
      SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger); /* To pick up result */
    };
  };
  
  if (!LastByte) {
    StatusReg=0x8c; /* Command busy, */
    UPDATENMISTATUS;
    SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger);
  };
}; /* ReadIDInterrupt */

/*--------------------------------------------------------------------------*/
static void Do128ByteSR_VerifyDataAndDeldCommand(void) {
  DoSelects();
  NotImp("Do128ByteSR_VerifyDataAndDeldCommand");
};
/*--------------------------------------------------------------------------*/
static void DoVarLength_VerifyDataAndDeldCommand(void) {
  int Drive=-1;

  DoSelects();

  if (!CheckReady()) {
    DoErr(0x10); /* Drive not ready */
    return;
  };

  if (Selects[0]) Drive=0;
  if (Selects[1]) Drive=1;

  Internal_CurrentTrack[Drive]=Params[0];
  CommandStatus.CurrentTrackPtr=GetTrackPtr(Params[0]);
  if (CommandStatus.CurrentTrackPtr==NULL) {
    DoErr(0x10);
    return;
  };

  CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,Params[1],0);
  if (CommandStatus.CurrentSectorPtr==NULL) {
    DoErr(0x1e); /* Sector not found */
    return;
  };

  StatusReg=0x80; /* Command busy */
  UPDATENMISTATUS;
  SetMotor(Drive,true);
  SetTrigger(100,Disc8271Trigger); /* A short delay to causing an interrupt */
};
/*--------------------------------------------------------------------------*/
static void VerifyInterrupt(void) {
  StatusReg=0x18; /* Result with interrupt */
  UPDATENMISTATUS;
  if (Selects[0])
    SetMotor(0,false);
  if (Selects[1])
    SetMotor(1,false);
  ResultReg=0; /* All OK */
};
/*--------------------------------------------------------------------------*/
static void DoFormatCommand(void) {
  int Drive=-1;

  DoSelects();

  DoLoadHead();

  if (!CheckReady()) {
    DoErr(0x10); /* Drive not ready */
    return;
  };

  if (Selects[0]) Drive=0;
  if (Selects[1]) Drive=1;

  if (!Writeable[Drive]) {
    DoErr(0x12); /* Drive write protected */
    return;
  };

  Internal_CurrentTrack[Drive]=Params[0];
  CommandStatus.CurrentTrackPtr=GetTrackPtr(Params[0]);
  if (CommandStatus.CurrentTrackPtr==NULL) {
    DoErr(0x10);
    return;
  };

  CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,0,0);
  if (CommandStatus.CurrentSectorPtr==NULL) {
    DoErr(0x1e); /* Sector not found */
    return;
  };

  CommandStatus.TrackAddr=Params[0];
  CommandStatus.CurrentSector=0;
  CommandStatus.SectorsToGo=Params[2] & 31;
  CommandStatus.SectorLength=1<<(7+((Params[2] >> 5) & 7));

  if (CommandStatus.SectorsToGo==10 && CommandStatus.SectorLength==256) {
    CommandStatus.ByteWithinSector=0;
    SetMotor(Drive,true);
    SetTrigger(TIMEBETWEENBYTES,Disc8271Trigger);
    StatusReg=0x80; /* Command busy */
    UPDATENMISTATUS;
    CommandStatus.ByteWithinSector=0;
    FirstWriteInt=1;
  } else {
    DoErr(0x1e); /* Sector not found */
  };
};

/*--------------------------------------------------------------------------*/
static void FormatInterrupt(void) {
  int i;
  int LastByte=0;

  if (CommandStatus.SectorsToGo<0) {
    StatusReg=0x18; /* Result and interrupt */
    UPDATENMISTATUS;
	if (Selects[0])
	  SetMotor(0,false);
	if (Selects[1])
	  SetMotor(1,false);
    return;
  };

  if (!FirstWriteInt) {
    /* Ignore the ID data for now - just count the bytes */
    CommandStatus.ByteWithinSector++;
  }
  else
    FirstWriteInt=0;

  ResultReg=0;
  if (CommandStatus.ByteWithinSector>=4) {
    /* Fill sector with 0xe5 chars */
    for (i=0; i<256; ++i)
      CommandStatus.CurrentSectorPtr->Data[i]=(unsigned char)0xe5;

    CommandStatus.ByteWithinSector=0;
    if (--CommandStatus.SectorsToGo) {
      CommandStatus.CurrentSector++;
      CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,CommandStatus.CurrentSector,0);
      if (CommandStatus.CurrentSectorPtr==NULL) {
        DoErr(0x1e); /* Sector not found */
        return;
      }
    } else {
      /* Last sector done, write the track back to disc */
		if (SaveTrackImage(Selects[0] ? 0 : 1, CURRENTHEAD, CommandStatus.TrackAddr)) {
      StatusReg=0x10;
      UPDATENMISTATUS;
      LastByte=1;
      CommandStatus.SectorsToGo=-1; /* To let us bail out */
      SetTrigger(0,Disc8271Trigger); /* To pick up result */
		} else {
			DoErr(0x12);
		}
    };
  };
  
  if (!LastByte) {
    StatusReg=0x8c; /* Command busy, */
    UPDATENMISTATUS;
    SetTrigger(TIMEBETWEENBYTES * 256,Disc8271Trigger);
  };
}; /* FormatInterrupt */

/*--------------------------------------------------------------------------*/
static void DoSeekInt(void) {
  StatusReg=0x18; /* Result with interrupt */
  UPDATENMISTATUS;
  if (Selects[0])
	SetMotor(0,false);
  if (Selects[1])
    SetMotor(1,false);
  ResultReg=0; /* All OK */
};
/*--------------------------------------------------------------------------*/
static void DoSeekCommand(void) {
  int Drive=-1;
  DoSelects();

  DoLoadHead();

  if (Selects[0]) Drive=0;
  if (Selects[1]) Drive=1;

  if (Drive<0) {
    DoErr(0x10);
    return;
  };
  
  Internal_CurrentTrack[Drive]=Params[0];

  StatusReg=0x80; /* Command busy */
  UPDATENMISTATUS;
  SetMotor(Drive,true);
  SetTrigger(100,Disc8271Trigger); /* A short delay to causing an interrupt */
};

/*--------------------------------------------------------------------------*/
static void DoReadDriveStatusCommand(void) {
	bool Track0 = false;
	bool WriteProt = false;
	
	if (ThisCommand & 0x40) {
		Track0=(Internal_CurrentTrack[0]==0);
		WriteProt=(!Writeable[0]);
	};
	
	if (ThisCommand & 0x80) {
		Track0=(Internal_CurrentTrack[1]==0);
		WriteProt=(!Writeable[1]);
	};
	
	ResultReg=0x80 | (Selects[1]?0x40:0) | (Selects[0]?0x4:0) | (Track0?2:0) | (WriteProt?8:0);
	StatusReg|=0x10; /* Result */
	UPDATENMISTATUS;
};

/*--------------------------------------------------------------------------*/
static void DoSpecifyCommand(void) {
  /* Should set stuff up here */
};
/*--------------------------------------------------------------------------*/
static void DoWriteSpecialCommand(void) {
  DoSelects();

  switch(Params[0]) {
    case 0x06:
      Internal_Scan_SectorNum=Params[1];
      break;

    case 0x14:
      Internal_Scan_Count&=0xff;
      Internal_Scan_Count|=Params[1]<<8;
      break;

    case 0x13:
      Internal_Scan_Count&=0xff00;
      Internal_Scan_Count|=Params[1];
      break;

    case 0x12:
      Internal_CurrentTrack[0]=Params[1];
      break;

    case 0x1a:
      Internal_CurrentTrack[1]=Params[1];
      break;

    case 0x17:
      Internal_ModeReg=Params[1];
      break;

    case 0x23:
      Internal_DriveControlOutputPort=Params[1];
      Selects[0]=(Params[1] & 0x40)!=0;
      Selects[1]=(Params[1] & 0x80)!=0;
      break;

    case 0x22:
      Internal_DriveControlInputPort=Params[1];
      break;

    case 0x10:
      Internal_BadTracks[0][0]=Params[1];
      break;

    case 0x11:
      Internal_BadTracks[0][1]=Params[1];
      break;

    case 0x18:
      Internal_BadTracks[1][0]=Params[1];
      break;

    case 0x19:
      Internal_BadTracks[1][1]=Params[1];
      break;

    default:
      /* cerr << "Write to bad special register\n"; */
      return;
      break;
  }; /* Special register number switch */

}; /* DoWriteSpecialCommand */

/*--------------------------------------------------------------------------*/
static void DoReadSpecialCommand(void) {
  DoSelects();

  switch(Params[0]) {
    case 0x06:
      ResultReg=Internal_Scan_SectorNum;
      break;

    case 0x14:
      ResultReg=(Internal_Scan_Count>>8) & 255;
      break;

    case 0x13:
      ResultReg=Internal_Scan_Count & 255;
      break;

    case 0x12:
      ResultReg=Internal_CurrentTrack[0];
      break;

    case 0x1a:
      ResultReg=Internal_CurrentTrack[1];
      break;

    case 0x17:
      ResultReg=Internal_ModeReg;
      break;

    case 0x23:
      ResultReg=Internal_DriveControlOutputPort;
      break;

    case 0x22:
      ResultReg=Internal_DriveControlInputPort;
      break;

    case 0x10:
      ResultReg=Internal_BadTracks[0][0];
      break;

    case 0x11:
      ResultReg=Internal_BadTracks[0][1];
      break;

    case 0x18:
      ResultReg=Internal_BadTracks[1][0];
      break;

    case 0x19:
      ResultReg=Internal_BadTracks[1][1];
      break;

    default:
      /* cerr << "Read of bad special register\n"; */
      return;
      break;
  }; /* Special register number switch */

  StatusReg |= 16; /* Result reg full */
  UPDATENMISTATUS;
};
/*--------------------------------------------------------------------------*/
static void DoBadCommand(void) {
};

/*--------------------------------------------------------------------------*/
/* The following table is used to parse commands from the command number written into
the command register - it can't distinguish between subcommands selected from the
first parameter */
static PrimaryCommandLookupType PrimaryCommandLookup[]={
  {0x00, 0x3f, 3, DoVarLength_ScanDataCommand, NULL,  "Scan Data (Variable Length/Multi-Record)"},
  {0x04, 0x3f, 3, DoVarLength_ScanDataAndDeldCommand, NULL,  "Scan Data & deleted data (Variable Length/Multi-Record)"},
  {0x0a, 0x3f, 2, Do128ByteSR_WriteDataCommand, NULL, "Write Data (128 byte/single record)"},
  {0x0b, 0x3f, 3, DoVarLength_WriteDataCommand, WriteInterrupt, "Write Data (Variable Length/Multi-Record)"},
  {0x0e, 0x3f, 2, Do128ByteSR_WriteDeletedDataCommand, NULL, "Write Deleted Data (128 byte/single record)"},
  {0x0f, 0x3f, 3, DoVarLength_WriteDeletedDataCommand, NULL, "Write Deleted Data (Variable Length/Multi-Record)"},
  {0x12, 0x3f, 2, Do128ByteSR_ReadDataCommand, NULL, "Read Data (128 byte/single record)"},
  {0x13, 0x3f, 3, DoVarLength_ReadDataCommand, ReadInterrupt, "Read Data (Variable Length/Multi-Record)"},
  {0x16, 0x3f, 2, Do128ByteSR_ReadDataAndDeldCommand, NULL, "Read Data & deleted data (128 byte/single record)"},
  {0x17, 0x3f, 3, DoVarLength_ReadDataAndDeldCommand, ReadInterrupt, "Read Data & deleted data (Variable Length/Multi-Record)"},
  {0x1b, 0x3f, 3, DoReadIDCommand, ReadIDInterrupt, "ReadID" },
  {0x1e, 0x3f, 2, Do128ByteSR_VerifyDataAndDeldCommand, NULL, "Verify Data and Deleted Data (128 byte/single record)"},
  {0x1f, 0x3f, 3, DoVarLength_VerifyDataAndDeldCommand, VerifyInterrupt, "Verify Data and Deleted Data (Variable Length/Multi-Record)"},
  {0x23, 0x3f, 5, DoFormatCommand, FormatInterrupt, "Format"},
  {0x29, 0x3f, 1, DoSeekCommand, DoSeekInt,    "Seek"},
  {0x2c, 0x3f, 0, DoReadDriveStatusCommand, NULL, "Read drive status"},
  {0x35, 0xff, 4, DoSpecifyCommand, NULL, "Specify" },
  {0x3a, 0x3f, 2, DoWriteSpecialCommand, NULL, "Write special registers" },
  {0x3d, 0x3f, 1, DoReadSpecialCommand, NULL, "Read special registers" },
  {0,    0,    0, DoBadCommand, NULL, "Unknown command"} /* Terminator due to 0 mask matching all */
}; /* PrimaryCommandLookup */

/*--------------------------------------------------------------------------*/
/* returns a pointer to the data structure for the given command            */
/* If no matching command is given, the pointer points to an entry with a 0 */
/* mask, with a sensible function to call.                                  */
static PrimaryCommandLookupType *CommandPtrFromNumber(int CommandNumber) {
  PrimaryCommandLookupType *presptr=PrimaryCommandLookup;

  for(;presptr->CommandNum!=(presptr->Mask & CommandNumber);presptr++);

  return(presptr);
}; /* CommandPtrFromNumber */

/*--------------------------------------------------------------------------*/
/* Address is in the range 0-7 - with the fe80 etc stripped out */
int Disc8271_read(int Address) {
  int Value=0;

	if (!Disc8271Enabled)
		return 0xFF;
	
	switch (Address) {
    case 0:
      /*cerr << "8271 Status register read (0x" << hex << int(StatusReg) << dec << ")\n"; */
      Value=StatusReg;
      break;

    case 1:
      /*cerr << "8271 Result register read (0x" << hex << int(ResultReg) << dec << ")\n"; */
      StatusReg &=~0x18; /* Clear interrupt request  and result reg full flag*/
      UPDATENMISTATUS;
      Value=ResultReg;
      ResultReg=0; /* Register goes to 0 after its read */
      break;

    case 4:
      /*cerr << "8271 data register read\n"; */
      StatusReg &= ~0xc; /* Clear interrupt and non-dma request - not stated but DFS never looks at result reg!*/
      UPDATENMISTATUS;
      Value=DataReg;
      break;

    default:
      /* cerr << "8271: Read to unknown register address=" << Address << "\n"; */
      break;
  }; /* Address switch */

  return(Value);
}; /* Disc8271_read */

/*--------------------------------------------------------------------------*/
static void CommandRegWrite(int Value) {
  PrimaryCommandLookupType *ptr=CommandPtrFromNumber(Value);
  /*cerr << "8271: Command register write value=0x" << hex << Value << dec << "(Name=" << ptr->Ident << ")\n"; */
  ThisCommand=Value;
  NParamsInThisCommand=ptr->NParams;
  PresentParam=0;

  StatusReg|=0x90; /* Observed on beeb for read special */
  UPDATENMISTATUS;

  /* No parameters then call routine immediatly */
  if (NParamsInThisCommand==0) {
    StatusReg&=0x7e;
    UPDATENMISTATUS;
    ptr->ToCall();
  };

}; /* CommandRegWrite */

/*--------------------------------------------------------------------------*/
static void ParamRegWrite(int Value) {
  if (PresentParam>=NParamsInThisCommand) {
    /* cerr << "8271: Unwanted parameter register write value=0x" << hex << Value << dec << "\n"; */
  } else {
    Params[PresentParam++]=Value;
    
    StatusReg&=0xfe; /* Observed on beeb */
    UPDATENMISTATUS;

    if (PresentParam>=NParamsInThisCommand) {

      StatusReg&=0x7e; /* Observed on beeb */
      UPDATENMISTATUS;

      PrimaryCommandLookupType *ptr=CommandPtrFromNumber(ThisCommand);
    /* cerr << "<Disc access>"; */
    /*  cerr << "8271: All parameters arrived for '" << ptr->Ident;
      int tmp;
      for(tmp=0;tmp<PresentParam;tmp++)
        cerr << " 0x" << hex << int(Params[tmp]);
      cerr << dec << "\n"; */

      ptr->ToCall();
    }; /* Got all params yet? */
  } /* Parameter wanted ? */
}; /* ParamRegWrite */

/*--------------------------------------------------------------------------*/
/* Address is in the range 0-7 - with the fe80 etc stripped out */
void Disc8271_write(int Address, int Value) {
	
	if (!Disc8271Enabled)
		return;
	
	// Clear a pending head unload
	if (DriveHeadUnloadPending) {
		DriveHeadUnloadPending = false;
		ClearTrigger(Disc8271Trigger);
	}
	
	switch (Address) {
    case 0:
      CommandRegWrite(Value);
      break;

    case 1:
      ParamRegWrite(Value);
      break;

    case 2:
      /* cerr << "8271: Reset register write, value=0x" << hex << Value << dec << "\n"; */
      /* The caller should write a 1 and then >11 cycles later a 0 - but I'm just going
      to reset on both edges */
      Disc8271_reset();
      break;

    case 4:
      /* cerr << "8271: data register write, value=0x" << hex << Value << dec << "\n"; */
      StatusReg &= ~0xc;
      UPDATENMISTATUS;
      DataReg=Value;
      break;

    default:
      /* cerr << "8271: Write to unknown register address=" << Address << ", value=0x" << hex << Value << dec << "\n"; */
      break;
  }; /* Address switch */

	DriveHeadScheduleUnload();
	
}; /* Disc8271_write */

/*--------------------------------------------------------------------------*/
static void DriveHeadScheduleUnload(void) {
	// Schedule head unload when nothing else is pending.
	// This is mainly for the sound effects, but it also marks the drives as
	// not ready when the motor stops.
	if (DriveHeadLoaded && Disc8271Trigger==CycleCountTMax) {
		SetTrigger(4000000,Disc8271Trigger); // 2s delay to unload
		DriveHeadUnloadPending = true;
	}
}

/*--------------------------------------------------------------------------*/
static bool DriveHeadMotorUpdate(void) {
	int Drive=0;
	int Cycles=0;
	int Tracks=0;
	
	if (DriveHeadUnloadPending) {
		// Mark drives as not ready
		Selects[0] = false;
		Selects[1] = false;
		DriveHeadUnloadPending = false;
		if (DriveHeadLoaded && DiscDriveSoundEnabled)
			PlaySoundSample(SAMPLE_HEAD_UNLOAD, false);
		DriveHeadLoaded = false;
		StopSoundSample(SAMPLE_DRIVE_MOTOR);
		StopSoundSample(SAMPLE_HEAD_SEEK);
		return true;
	}
	
	if (!DiscDriveSoundEnabled)
	{
		DriveHeadLoaded = true;
		return false;
	}
	
	if (!DriveHeadLoaded) {
		PlaySoundSample(SAMPLE_DRIVE_MOTOR, true);
		DriveHeadLoaded = true;
		PlaySoundSample(SAMPLE_HEAD_LOAD, false);
		Cycles = SAMPLE_HEAD_LOAD_CYCLES;
		SetTrigger(Cycles,Disc8271Trigger);
		return true;
	}
	
	if (Selects[0]) Drive=0;
	if (Selects[1]) Drive=1;
	StopSoundSample(SAMPLE_HEAD_SEEK);
	if (DriveHeadPosition[Drive] != Internal_CurrentTrack[Drive]) {
		Tracks = abs(DriveHeadPosition[Drive] - Internal_CurrentTrack[Drive]);
		if (Tracks > 1) {
			PlaySoundSample(SAMPLE_HEAD_SEEK, true);
			Cycles = Tracks * SAMPLE_HEAD_SEEK_CYCLES_PER_TRACK;
		}
		else {
			PlaySoundSample(SAMPLE_HEAD_STEP, false);
			Cycles = SAMPLE_HEAD_STEP_CYCLES;
		}
		if (DriveHeadPosition[Drive] < Internal_CurrentTrack[Drive])
			DriveHeadPosition[Drive] += Tracks;
		else
			DriveHeadPosition[Drive] -= Tracks;
		SetTrigger(Cycles,Disc8271Trigger);
		return true;
	}
	return false;}



/*--------------------------------------------------------------------------*/
void Disc8271_poll_real(void) {
  ClearTrigger(Disc8271Trigger);
	
	if (DriveHeadMotorUpdate())
		return;
	
  /* Set the interrupt flag in the status register */
  StatusReg|=8;
  UPDATENMISTATUS;

  if (NextInterruptIsErr) {
    ResultReg=NextInterruptIsErr;
    StatusReg=0x18; /* ResultReg full and interrupt */
    UPDATENMISTATUS;
    NextInterruptIsErr=0;
  } else {
    /* Should only happen while a command is still active */
	  PrimaryCommandLookupType *comptr;
    comptr=CommandPtrFromNumber(ThisCommand);
    if (comptr->IntHandler!=NULL) comptr->IntHandler();
  };
	
	 DriveHeadScheduleUnload();
	
}; /* Disc8271_poll */

/*--------------------------------------------------------------------------*/
/* Checks it the sectors passed in look like a valid disc catalogue. Returns:
      1 - looks like a catalogue
      0 - does not look like a catalogue
     -1 - cannot tell
*/
static int CheckForCatalogue(unsigned char *Sec1, unsigned char *Sec2) {
  int Valid=1;
  int CatEntries = 0;
  int File;
  unsigned char c;
  int Invalid;
	
  /* First check the number of sectors (cannot be > 0x320) */
  if (((Sec2[6]&3)<<8)+Sec2[7] > 0x320)
    Valid=0;

  /* Check the number of catalogue entries (must be multiple of 8) */
  if (Valid)
  {
    if (Sec2[5] % 8)
      Valid=0;
    else
      CatEntries = Sec2[5] / 8;
  }

  /* Check that the catalogue file names are all printable characters. */
	Invalid = 0;
	for (File=0; Valid && File<CatEntries; ++File) {
    for (int i=0; Valid && i<8; ++i) {
      c=Sec1[8+File*8+i];

      if (i==7)  /* Remove lock bit */
        c&=0x7f;

      if (c<0x20 || c>0x7f)
        Invalid++;  /* not printable */
    }
  }

	/* Some games discs have one or two invalid names */
	if (Invalid > 3)
		Valid=0;
	
	
#if 0
	/* Check that all the bytes after the file names are 0 */
  for (File=CatEntries; Valid && File<31; ++File) {
    for (int i=0; Valid && i<8; ++i) {
      c=Sec1[8+File*8+i];

      if (c!=0)
        Valid=0;
    }
  }
#endif
	
  /* If still valid but there are no catalogue entries then we cannot tell
     if its a catalog */
  if (Valid && CatEntries==0)
    Valid=-1;

  return Valid;
}

/*--------------------------------------------------------------------------*/
void FreeDiscImage(int DriveNum) {
  int Track,Head,Sector;
  SectorType *SecPtr;

  for(Track=0; Track<TRACKSPERDRIVE; Track++) {
    for(Head=0; Head<2; Head++) {
      SecPtr=DiscStore[DriveNum][Head][Track].Sectors;
      if (SecPtr != NULL) {
        for(Sector=0; Sector<10; Sector++) {
          if (SecPtr[Sector].Data != NULL)
          {
            free(SecPtr[Sector].Data);
            SecPtr[Sector].Data=NULL;
          }
        }
        free(SecPtr);
        DiscStore[DriveNum][Head][Track].Sectors=NULL;
      }
    }
  }
}
      
/*--------------------------------------------------------------------------*/
void LoadSimpleDiscImage(char *FileName, int DriveNum,int HeadNum, int Tracks) {
  int CurrentTrack,CurrentSector;
  SectorType *SecPtr;
  char buff[256];
  
  strcpy(buff, FileName);
  
  fprintf(stderr, "Loading disc file %s\n", buff);

  FILE *infile=fopen(buff,"rb");
  
  if (!infile) {
#ifdef WIN32
    char errstr[200];
    sprintf(errstr, "Could not open disc file:\n  %s", FileName);
    MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
#else
    fprintf(stderr, "Could not open disc file %s\n", FileName);
#endif
    return;
  };

  mainWin->SetImageName(FileName,DriveNum,0);

  strcpy(FileNames[DriveNum], FileName);
  NumHeads[DriveNum] = 1;

  FreeDiscImage(DriveNum);

  for(CurrentTrack=0;CurrentTrack<Tracks;CurrentTrack++) {
    DiscStore[DriveNum][HeadNum][CurrentTrack].LogicalSectors=10;
    DiscStore[DriveNum][HeadNum][CurrentTrack].NSectors=10;
    SecPtr=DiscStore[DriveNum][HeadNum][CurrentTrack].Sectors=(SectorType*)calloc(10,sizeof(SectorType));
    DiscStore[DriveNum][HeadNum][CurrentTrack].Gap1Size=0; /* Don't bother for the mo */
    DiscStore[DriveNum][HeadNum][CurrentTrack].Gap3Size=0;
    DiscStore[DriveNum][HeadNum][CurrentTrack].Gap5Size=0;

    for(CurrentSector=0;CurrentSector<10;CurrentSector++) {
      SecPtr[CurrentSector].IDField.CylinderNum=CurrentTrack;     
      SecPtr[CurrentSector].IDField.RecordNum=CurrentSector;
      SecPtr[CurrentSector].IDField.HeadNum=HeadNum;
      SecPtr[CurrentSector].IDField.PhysRecLength=256;
      SecPtr[CurrentSector].Deleted=0;
      SecPtr[CurrentSector].Data=(unsigned char *)calloc(1,256);
      fread(SecPtr[CurrentSector].Data,1,256,infile);
    }; /* Sector */
  }; /* Track */

  fclose(infile);

  /* Check if the sectors that would be the disc catalogue of a double sized
     image look like a disc catalogue - give a warning if they do. */
  if (CheckForCatalogue(DiscStore[DriveNum][HeadNum][1].Sectors[0].Data,
                        DiscStore[DriveNum][HeadNum][1].Sectors[1].Data) == 1) {
#ifdef WIN32
    MessageBox(GETHWND,"WARNING - Incorrect disc type selected?\n\n"
                       "This disc file looks like a double sided\n"
                       "disc image. Check files before copying them.\n",
                       "BBC Emulator",MB_OK|MB_ICONWARNING);
#else
    fprintf(stderr, "WARNING - Incorrect disc type selected(?) in drive %d\n", DriveNum);
    fprintf(stderr, "This disc file looks like a double sided disc image.\n");
    fprintf(stderr, "Check files before copying them.\n");
#endif
  }
};  /* LoadSimpleDiscImage */

/*--------------------------------------------------------------------------*/
void LoadSimpleDSDiscImage(char *FileName, int DriveNum,int Tracks) {

  char buff[256];
  
  sprintf(buff, "%s", FileName);
  
  fprintf(stderr, "Loading DSD disc file %s\n", buff);

  FILE *infile=fopen(buff,"rb");
  int CurrentTrack,CurrentSector,HeadNum;
  SectorType *SecPtr;

  if (!infile) {
#ifdef WIN32
    char errstr[200];
    sprintf(errstr, "Could not open disc file:\n  %s", FileName);
    MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
#else
    fprintf(stderr, "Could not open disc file %s\n", FileName);
#endif
    return;
  };

  mainWin->SetImageName(FileName,DriveNum,1);

  strcpy(FileNames[DriveNum], FileName);
  NumHeads[DriveNum] = 2;

  FreeDiscImage(DriveNum);

  for(CurrentTrack=0;CurrentTrack<Tracks;CurrentTrack++) {
    for(HeadNum=0;HeadNum<2;HeadNum++) {
      DiscStore[DriveNum][HeadNum][CurrentTrack].LogicalSectors=10;
      DiscStore[DriveNum][HeadNum][CurrentTrack].NSectors=10;
      SecPtr=DiscStore[DriveNum][HeadNum][CurrentTrack].Sectors=(SectorType *)calloc(10,sizeof(SectorType));
      DiscStore[DriveNum][HeadNum][CurrentTrack].Gap1Size=0; /* Don't bother for the mo */
      DiscStore[DriveNum][HeadNum][CurrentTrack].Gap3Size=0;
      DiscStore[DriveNum][HeadNum][CurrentTrack].Gap5Size=0;

      for(CurrentSector=0;CurrentSector<10;CurrentSector++) {
        SecPtr[CurrentSector].IDField.CylinderNum=CurrentTrack;     
        SecPtr[CurrentSector].IDField.RecordNum=CurrentSector;
        SecPtr[CurrentSector].IDField.HeadNum=HeadNum;
        SecPtr[CurrentSector].IDField.PhysRecLength=256;
        SecPtr[CurrentSector].Deleted=0;
        SecPtr[CurrentSector].Data=(unsigned char *)calloc(1,256);
        fread(SecPtr[CurrentSector].Data,1,256,infile);
      }; /* Sector */
    }; /* Head */
  }; /* Track */

  fclose(infile);

  /* Check if the side 2 catalogue sectors look OK - give a warning if they do not. */
  if (CheckForCatalogue(DiscStore[DriveNum][1][0].Sectors[0].Data,
                        DiscStore[DriveNum][1][0].Sectors[1].Data) == 0) {
#ifdef WIN32
    MessageBox(GETHWND,"WARNING - Incorrect disc type selected?\n\n"
                       "This disc file looks like a single sided\n"
                       "disc image. Check files before copying them.\n",
                       "BBC Emulator",MB_OK|MB_ICONWARNING);
#else
    fprintf(stderr, "WARNING - Incorrect disc type selected(?) in drive %d\n", DriveNum);
    fprintf(stderr, "This disc file looks like a single sided disc image.\n");
    fprintf(stderr, "Check files before copying them.\n");
#endif
  }
};  /* LoadSimpleDSDiscImage */

/*--------------------------------------------------------------------------*/
void Eject8271DiscImage(int DriveNum) {
	strcpy(FileNames[DriveNum], "");
	FreeDiscImage(DriveNum);
}

/*--------------------------------------------------------------------------*/
static bool SaveTrackImage(int DriveNum, int HeadNum, int TrackNum) {
  int dSuccess=1;
  int CurrentSector;
  long FileOffset;
  long FileLength;
  SectorType *SecPtr;

  FILE *outfile=fopen(FileNames[DriveNum],"r+b");

  if (!outfile) {
#ifdef WIN32
    char errstr[200];
    sprintf(errstr, "Could not open disc file for write:\n  %s", FileNames[DriveNum]);
    MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
#else
    fprintf(stderr, "Could not open disc file for write %s\n", FileNames[DriveNum]);
#endif
    return false;
  };

  FileOffset=(NumHeads[DriveNum]*TrackNum+HeadNum)*2560;

  /* Get the file length to check if the file needs extending */
  dSuccess = !fseek(outfile, 0L, SEEK_END);
  if (dSuccess)
  {
    FileLength=ftell(outfile);
    if (FileLength == -1L)
      dSuccess=0;
  }
  while (dSuccess && FileOffset > FileLength)
  {
    if (fputc(0, outfile) == EOF)
      dSuccess=0;
    FileLength++;
  }
  if (dSuccess)
  {
    dSuccess = !fseek(outfile, FileOffset, SEEK_SET);

    SecPtr=DiscStore[DriveNum][HeadNum][TrackNum].Sectors;
    for(CurrentSector=0;dSuccess && CurrentSector<10;CurrentSector++) {
      if (fwrite(SecPtr[CurrentSector].Data,1,256,outfile) != 256)
        dSuccess=0;
    }
  }

  if (fclose(outfile) != 0)
    dSuccess=0;

  if (!dSuccess) {
#ifdef WIN32
    char errstr[200];
    sprintf(errstr, "Failed writing to disc file:\n  %s", FileNames[DriveNum]);
    MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
#else
    fprintf(stderr, "Failed writing to disc file %s\n", FileNames[DriveNum]);
#endif
  };
	return dSuccess != 0;
};  /* SaveTrackImage */

/*--------------------------------------------------------------------------*/
int IsDiscWritable(int DriveNum) {
  return Writeable[DriveNum];
}

/*--------------------------------------------------------------------------*/
void DiscWriteEnable(int DriveNum, int WriteEnable) {
  int HeadNum;
  SectorType *SecPtr;
  unsigned char *Data;
  int File;
  int Catalogue, NumCatalogues;
  int NumSecs;
  int StartSec, LastSec;
  int DiscOK=1;

  Writeable[DriveNum]=WriteEnable != 0;

  /* If disc is being made writable then check that the disc catalogue will
     not get corrupted if new files are added.  The files in the disc catalogue
     must be in descending sector order otherwise the DFS ROMs write over
     files at the start of the disc.  The sector count in the catalogue must
     also be correct. */
  if (WriteEnable) {
    for(HeadNum=0; DiscOK && HeadNum<NumHeads[DriveNum]; HeadNum++) {
      SecPtr=DiscStore[DriveNum][HeadNum][0].Sectors;
      if (SecPtr==NULL)
        return; /* No disc image! */

      Data=SecPtr[1].Data;

      /* Check for a Watford DFS 62 file catalogue */
      NumCatalogues=2;
      Data=SecPtr[2].Data;
      for (int i=0; i<8; ++i)
        if (Data[i]!=(unsigned char)0xaa) {
          NumCatalogues=1;
          break;
        }

      for (Catalogue=0; DiscOK && Catalogue<NumCatalogues; ++Catalogue) {
        Data=SecPtr[Catalogue*2+1].Data;

        /* First check the number of sectors */
        NumSecs=((Data[6]&3)<<8)+Data[7];
        if (NumSecs != 0x320 && NumSecs != 0x190) {
          DiscOK=0;
        } else {

          /* Now check the start sectors of each file */
          LastSec=0x320;
          for (File=0; DiscOK && File<Data[5]/8; ++File) {
            StartSec=((Data[File*8+14]&3)<<8)+Data[File*8+15];
            if (LastSec < StartSec)
              DiscOK=0;
            LastSec=StartSec;
          }
        } /* if num sectors OK */
      } /* for catalogue */
    } /* for disc head */

    if (!DiscOK)
    {
#ifdef WIN32
      MessageBox(GETHWND,"WARNING - Invalid Disc Catalogue\n\n"
                       "This disc image will get corrupted if\n"
                       "files are written to it.  Copy all the\n"
                       "files to a new image to fix it.",
                       "BBC Emulator",MB_OK|MB_ICONWARNING);
#else
      fprintf(stderr, "WARNING - Invalid Disc Catalogue in drive %d\n", DriveNum);
      fprintf(stderr, "This disc image will get corrupted if files are written to it.\n");
      fprintf(stderr, "Copy all the files to a new image to fix it.\n");
#endif
    }

  } /* if write enabled */

} /* DiscWriteEnable */

/*--------------------------------------------------------------------------*/
void CreateDiscImage(char *FileName, int DriveNum, int Heads, int Tracks) {
  int dSuccess=1;
  int Sector;
  int NumSectors;
  int i;
  FILE *outfile;
  unsigned char SecData[256];

  /* First check if file already exists */
  outfile=fopen(FileName,"rb");
  if (outfile != NULL) {
    fclose(outfile);
#ifdef WIN32
    char errstr[200];
    sprintf(errstr, "File already exists:\n  %s\n\nOverwrite file?", FileName);
    if (MessageBox(GETHWND,errstr,"BBC Emulator",MB_YESNO|MB_ICONQUESTION) != IDYES)
      return;
#else
//    cerr << "Could not create disc file " << FileName << "\n";
//    return;
#endif
  };

  outfile=fopen(FileName,"wb");
  if (!outfile) {
#ifdef WIN32
    char errstr[200];
    sprintf(errstr, "Could not create disc file:\n  %s", FileName);
    MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
#else
    fprintf(stderr, "Could not create disc file %s\n", FileName);
#endif
    return;
  };

  NumSectors=Tracks*10;

  /* Create the first two secotrs on each side - the rest will get created when
     data is written to it. */
  for(Sector=0;dSuccess && Sector<(Heads==1?2:12);Sector++) {
    for (i=0;i<256;++i)
      SecData[i]=0;

    if (Sector==1 || Sector==11)
    {
      SecData[6]=NumSectors >> 8;
      SecData[7]=NumSectors & 0xff;
    }

    if (fwrite(SecData,1,256,outfile) != 256)
      dSuccess=0;
  } /* Sector */

  if (fclose(outfile) != 0)
    dSuccess=0;

  if (!dSuccess) {
#ifdef WIN32
    char errstr[200];
    sprintf(errstr, "Failed writing to disc file:\n  %s", FileNames);
    MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
#else
    fprintf(stderr, "Failed writing to disc file %s\n", FileName);
#endif
  }
  else
  {
    /* Now load the new image into the correct drive */
    if (Heads==1)
	{
		if ((MachineType == 3) || (!NativeFDC))
			Load1770DiscImage(FileName, DriveNum, 0);
		else
			LoadSimpleDiscImage(FileName, DriveNum, 0, Tracks);
	}
    else
	{
		if ((MachineType == 3) || (!NativeFDC))
			Load1770DiscImage(FileName, DriveNum, 1);
		else
			LoadSimpleDSDiscImage(FileName, DriveNum, Tracks);
	}
  }
};  /* CreateDiscImage */

/*--------------------------------------------------------------------------*/
void LoadStartupDisc(int DriveNum, char *DiscString) {
  char DoubleSided;
  int Tracks;
  char Name[1024];

  Tracks = 80;
  DoubleSided = 'D';
  strcpy(Name, DiscString);
  
//  if (scanfres=sscanf(DiscString,"%c:%d:%s",&DoubleSided,&Tracks,Name),

    fprintf(stderr, "Loading %c, %d, disc file %s\n", DoubleSided, Tracks, Name);

	if (strstr(Name, "ssd") > (char*)0) DoubleSided = 'S';
	if (strstr(Name, "dsd") > (char*)0) DoubleSided = 'D';
	if (strstr(Name, "img") > (char*)0) DoubleSided = 'S';
	if (strstr(Name, "adl") > (char*)0) DoubleSided = 'A';
	if (strstr(Name, "adf") > (char*)0) DoubleSided = 'A';

	if (strstr(Name, "SSD") > (char*)0) DoubleSided = 'S';
	if (strstr(Name, "DSD") > (char*)0) DoubleSided = 'D';
	if (strstr(Name, "IMG") > (char*)0) DoubleSided = 'S';
	if (strstr(Name, "ADL") > (char*)0) DoubleSided = 'A';
	if (strstr(Name, "ADF") > (char*)0) DoubleSided = 'A';

	
    switch (DoubleSided) {
      case 'd':
      case 'D':
        if ((MachineType==3) || (!NativeFDC))
          Load1770DiscImage(Name,DriveNum,1);
        else
          LoadSimpleDSDiscImage(Name,DriveNum,Tracks);
        break;

      case 'S':
      case 's':
        if ((MachineType==3) || (!NativeFDC))
          Load1770DiscImage(Name,DriveNum,0);
        else
          LoadSimpleDiscImage(Name,DriveNum,0,Tracks);
        break;

      case 'A':
      case 'a':
        if ((MachineType==3) || (!NativeFDC)) 
          Load1770DiscImage(Name,DriveNum,2);
        else
          fprintf(stderr, "The 8271 FDC Cannot load the ADFS disc image specified in the BeebDiscLoad environment variable\n");
        break;

      default:
#ifdef WIN32
        MessageBox(GETHWND,"BeebDiscLoad disc type incorrect, use S for single sided, "
                   "D for double sided and A for ADFS", "BBC Emulator",MB_OK|MB_ICONERROR);
#else
        fprintf(stderr, "BeebDiscLoad environment variable set wrong - the\n");
        fprintf(stderr, "first character is either S or D signifying\n");
        fprintf(stderr, "single or double sided\n");
#endif
        break;        
    }; /* Switch */
} /* LoadStartupDisc */

/*--------------------------------------------------------------------------*/
void Disc8271_reset(void) {
  static int onetime_initdisc=0;
  char *DiscString;

  ResultReg=0;
  StatusReg=0;
  UPDATENMISTATUS;
  Internal_Scan_SectorNum=0;
  Internal_Scan_Count=0; /* Read as two bytes */
  Internal_ModeReg=0;
  Internal_CurrentTrack[0]=Internal_CurrentTrack[1]=0; /* 0/1 for surface number */
  Internal_DriveControlOutputPort=0;
  Internal_DriveControlInputPort=0;
  Internal_BadTracks[0][0]=Internal_BadTracks[0][1]=Internal_BadTracks[1][0]=Internal_BadTracks[1][1]=0xff; /* 1st subscript is surface 0/1 and second subscript is badtrack 0/1 */
	if (DriveHeadLoaded) {
		DriveHeadUnloadPending = true;
		DriveHeadMotorUpdate();
	}
	ClearTrigger(Disc8271Trigger); /* No Disc8271Triggered events yet */

  ThisCommand=-1;
  NParamsInThisCommand=0;
  PresentParam=0;
  Selects[0]=Selects[1]=false;

  if (!onetime_initdisc) {
    onetime_initdisc++;
    InitDiscStore();

    DiscString=getenv("BeebDiscLoad");
    if (DiscString==NULL)
      DiscString=getenv("BeebDiscLoad0");
    if (DiscString!=NULL)
      LoadStartupDisc(0, DiscString);
    else {
//      LoadStartupDisc(0, "S:80:arcadians.ssd");
    }

    DiscString=getenv("BeebDiscLoad1");
    if (DiscString!=NULL)
      LoadStartupDisc(1, DiscString);

    if (getenv("BeebDiscWrites")!=NULL) {
      DiscWriteEnable(0, 1);
      DiscWriteEnable(1, 1);
    }
  }; /* One time discload */

}; /* Disc8271_reset */

/*--------------------------------------------------------------------------*/
void disc8271_dumpstate(void) {
};

/*--------------------------------------------------------------------------*/

void Save8271UEF(FILE *SUEF)
{
	char blank[256];
	memset(blank,0,256);

	fput16(0x046E,SUEF);
	fput32(613,SUEF);

	if (DiscStore[0][0][0].Sectors == NULL) {
		// No disc in drive 0
		fwrite(blank,1,256,SUEF);
	}
	else {
		fwrite(FileNames[0],1,256,SUEF);
	}
	if (DiscStore[1][0][0].Sectors == NULL) {
		// No disc in drive 1
		fwrite(blank,1,256,SUEF);
	}
	else {
		fwrite(FileNames[1],1,256,SUEF);
	}

	if (Disc8271Trigger == CycleCountTMax)
		fput32(Disc8271Trigger,SUEF);
	else
		fput32(Disc8271Trigger - TotalCycles,SUEF);
	fputc(ResultReg,SUEF);
	fputc(StatusReg,SUEF);
	fputc(DataReg,SUEF);
	fputc(Internal_Scan_SectorNum,SUEF);
	fput32(Internal_Scan_Count,SUEF);
	fputc(Internal_ModeReg,SUEF);
	fputc(Internal_CurrentTrack[0],SUEF);
	fputc(Internal_CurrentTrack[1],SUEF);
	fputc(Internal_DriveControlOutputPort,SUEF);
	fputc(Internal_DriveControlInputPort,SUEF);
	fputc(Internal_BadTracks[0][0],SUEF);
	fputc(Internal_BadTracks[0][1],SUEF);
	fputc(Internal_BadTracks[1][0],SUEF);
	fputc(Internal_BadTracks[1][1],SUEF);
	fput32(ThisCommand,SUEF);
	fput32(NParamsInThisCommand,SUEF);
	fput32(PresentParam,SUEF);
	fwrite(Params,1,16,SUEF);
	fput32(NumHeads[0],SUEF);
	fput32(NumHeads[1],SUEF);
	fput32(Selects[0]?1:0,SUEF);
	fput32(Selects[1]?1:0,SUEF);
	fput32(Writeable[0]?1:0,SUEF);
	fput32(Writeable[1]?1:0,SUEF);
	fput32(FirstWriteInt,SUEF);
	fput32(NextInterruptIsErr,SUEF);
	fput32(CommandStatus.TrackAddr,SUEF);
	fput32(CommandStatus.CurrentSector,SUEF);
	fput32(CommandStatus.SectorLength,SUEF);
	fput32(CommandStatus.SectorsToGo,SUEF);
	fput32(CommandStatus.ByteWithinSector,SUEF);
}

void Load8271UEF(FILE *SUEF)
{
	extern bool DiscLoaded[2];
	char FileName[256];
	char *ext;
	int Loaded=0;
	int LoadFailed=0;

	// Clear out current images, don't want them corrupted if
	// saved state was in middle of writing to disc.
	FreeDiscImage(0);
	FreeDiscImage(1);
	DiscLoaded[0]=FALSE;
	DiscLoaded[1]=FALSE;

	fread(FileName,1,256,SUEF);
	if (FileName[0]) {
		// Load drive 0
		Loaded=1;
		ext = strrchr(FileName, '.');
		if (ext != NULL && strcmp(ext+1, "dsd") == 0)
			LoadSimpleDSDiscImage(FileName, 0, 80);
		else
			LoadSimpleDiscImage(FileName, 0, 0, 80);

		if (DiscStore[0][0][0].Sectors == NULL)
			LoadFailed=1;
	}

	fread(FileName,1,256,SUEF);
	if (FileName[0]) {
		// Load drive 1
		Loaded=1;
		ext = strrchr(FileName, '.');
		if (ext != NULL && strcmp(ext+1, "dsd") == 0)
			LoadSimpleDSDiscImage(FileName, 1, 80);
		else
			LoadSimpleDiscImage(FileName, 1, 0, 80);

		if (DiscStore[1][0][0].Sectors == NULL)
			LoadFailed=1;
	}

	if (Loaded && !LoadFailed)
	{
		Disc8271Trigger=fget32(SUEF);
		if (Disc8271Trigger != CycleCountTMax)
			Disc8271Trigger+=TotalCycles;

		ResultReg=fgetc(SUEF);
		StatusReg=fgetc(SUEF);
		DataReg=fgetc(SUEF);
		Internal_Scan_SectorNum=fgetc(SUEF);
		Internal_Scan_Count=fget32(SUEF);
		Internal_ModeReg=fgetc(SUEF);
		Internal_CurrentTrack[0]=fgetc(SUEF);
		Internal_CurrentTrack[1]=fgetc(SUEF);
		Internal_DriveControlOutputPort=fgetc(SUEF);
		Internal_DriveControlInputPort=fgetc(SUEF);
		Internal_BadTracks[0][0]=fgetc(SUEF);
		Internal_BadTracks[0][1]=fgetc(SUEF);
		Internal_BadTracks[1][0]=fgetc(SUEF);
		Internal_BadTracks[1][1]=fgetc(SUEF);
		ThisCommand=fget32(SUEF);
		NParamsInThisCommand=fget32(SUEF);
		PresentParam=fget32(SUEF);
		fread(Params,1,16,SUEF);
		NumHeads[0]=fget32(SUEF);
		NumHeads[1]=fget32(SUEF);
		Selects[0]=fget32(SUEF) != 0;
		Selects[1]=fget32(SUEF) != 0;
		Writeable[0]=fget32(SUEF) != 0;
		Writeable[1]=fget32(SUEF) != 0;
		FirstWriteInt=fget32(SUEF);
		NextInterruptIsErr=fget32(SUEF);
		CommandStatus.TrackAddr=fget32(SUEF);
		CommandStatus.CurrentSector=fget32(SUEF);
		CommandStatus.SectorLength=fget32(SUEF);
		CommandStatus.SectorsToGo=fget32(SUEF);
		CommandStatus.ByteWithinSector=fget32(SUEF);

		CommandStatus.CurrentTrackPtr=GetTrackPtr(CommandStatus.TrackAddr);
		if (CommandStatus.CurrentTrackPtr!=NULL)
			CommandStatus.CurrentSectorPtr=GetSectorPtr(CommandStatus.CurrentTrackPtr,CommandStatus.CurrentSector,0);
		else
			CommandStatus.CurrentSectorPtr=NULL;
	}
}


/*--------------------------------------------------------------------------*/
void Get8271DiscInfo(int DriveNum, char *pFileName, int *Heads)
{
	strcpy(pFileName, FileNames[DriveNum]);
	*Heads = NumHeads[DriveNum];
}

