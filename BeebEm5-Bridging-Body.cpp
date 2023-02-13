//
//  BeebEm5-Bridging-Body.cpp
//  BeebEm5
//
//  Created by Commander Coder on 04/01/2021.
//  Copyright © 2021 Andrew Hague. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Carbon/Carbon.h>

#include "main.h"
#include "beebwin.h"
#include "6502core.h"
#include "beebsound.h"
#include "serial.h"
#include "speech.h"
#include "tube.h"
#include "uef.h"
#include "csw.h"
#include "uservia.h"

int done = 0;
extern OSStatus TCWindowCommandHandler(UInt32 cmdID);
extern FILE *tlog;
extern unsigned char UEFOpen;


extern BeebWin *mainWin;

extern char AutoRunPath[];
extern void AutoRunFromPath();

extern "C" void beeb_autorun(char* path)
{
    strcpy(AutoRunPath, path);
}

extern "C" void beeb_handlemouse(long eventkind)
{
  // here
    fprintf(stderr, "eventKind is %ld\n", eventkind);
    switch (eventkind)
    {
        // TODO: see main.cc for how it was done previously
        case mouseDown:
            AMXButtons |= AMX_LEFT_BUTTON;
            break;
        case mouseUp:
            AMXButtons &= ~AMX_LEFT_BUTTON;
            break;
        case 100:
            AMXButtons |= AMX_RIGHT_BUTTON;
            break;
        case 99:
            AMXButtons &= ~AMX_RIGHT_BUTTON;
            break;
            
    }
}

extern "C" void beeb_SetAMXPosition(unsigned int x, unsigned int y)
{
    // TODO: Put call to BeebWin::SetAMXPosition(x,y) here but how?
    mainWin->SetAMXPosition(x, y);
}

extern "C" void beeb_handlekeys(long eventkind, unsigned int keycode, char charCode)
{
     static int ctrl = 0x0000;
     int LastShift, LastCtrl, LastCaps, LastCmd;
     int NewShift, NewCtrl = 0, NewCaps;
//     static int NewCmd = 0; //remember CMD pressed
     
    
    switch (eventkind)
    {
        case kEventRawKeyDown:

// ALL THESE HANDLE BY 'beeb_HandleCommand' since CMD-C and CMD-V are set to 'copy' and 'past'
// Full screen is on CMD-CTRL-F
// CMD-CTRL-S on PRINTSCREEN - 'swtd'
// CMD-T is CMD-CTRL-T on TRACE
#if 0 //ACH
//              fprintf(stderr, "Key pressed: code = %d, '%c'\n", keycode, charCode);
                if ( (NewCmd) && (keycode == 6) ) // 6 = Z key
                {
                    fprintf(stderr, "cmd-Z pressed, NewCmd = %d\n", NewCmd);
                    if (mainWin->m_isFullScreen)
                    {

 //                       EndFullScreen(mainWin->m_RestoreState, nil);
                        mainWin->m_isFullScreen = 0;
                        mainWin->SetMenuCommandIDCheck('vfsc', false);
                    }
                } else if ( (NewCmd) && (NewCtrl) && (keycode == 1) ) // 1 = S key
                {
                    fprintf(stderr, "ctrl-cmd-S pressed, NewCmd = %d\n", NewCmd);
//                    mainWin->m_PrintScreen = true;
                } else if ( (NewCmd) && (keycode == 8) ) // 8 = C key
                {
                    fprintf(stderr, "cmd-C pressed, NewCmd = %d\n", NewCmd);
//                    mainWin->doCopy();
                } else if ( (NewCmd) && (keycode == 9) ) // 9 = V key
                {
                    fprintf(stderr, "cmd-V pressed, NewCmd = %d\n", NewCmd);
//                    mainWin->doPaste();
                } else if ( (NewCmd) && (keycode == 17) ) // 17 = T key
                {
                    // CMD-T
                    fprintf(stderr, "cmd-T pressed, NewCmd = %d\n", NewCmd);
                    trace_186 = 1 - trace_186;
                }
              else
#endif
                {
                    mainWin->KeyDown(keycode);
                }
            break;
          case kEventRawKeyUp:
//            fprintf(stderr, "Key released: code = %d, '%c'\n", keycode, charCode);
            mainWin->KeyUp(keycode);
            break;
          case kEventRawKeyModifiersChanged:
//            fprintf(stderr, "Key Modifier: code = %d, '%c'\n", keycode, charCode);

            /* Cocoa Event Handling used to be:
             cmd 0x0000
             shift 0x0200
             alphalock 0x0400
             option 0x0800
             ctrl 0x1000
             rightcmd 0x0001
             rightshift 0x2000
             rightoption 0x4000
             rightctrl 0x8000
             */
            
            /* new codes are (left/right or toggle):
             cmd 0x100008
             shift 0x20002
             alphalock 0x10000 & 0x00000  - toggle on off - unused as keycodes from mac will be capslocked too
             option 0x80020
             ctrl 0x40001
             rightshift 0x20004
             rightoption 0x80040 - available for COPY
             rightctrl 0x42001 - unused as only one ctrl key
             rightcmd 0x100010 - available for COPY
             fn  0x800000
             
             */
                // ALPHALOCK key is unused
            
                LastShift = ctrl & 0x20006; // capture left and right shift
                LastCtrl = ctrl & 0x40001; // capture left
                LastCaps = ctrl & 0x80020; // capture left ALT/Option
//                LastCmd = ctrl & 0x100018; // capture left and right CMD

                NewShift = keycode & 0x20006; // capture left and right shift
                NewCtrl = keycode & 0x40001; // capture left
                NewCaps = keycode & 0x80020; // capture left ALT/Option
//                NewCmd = keycode &  0x100018; // capture left and right CMD

            
//                fprintf(stderr, "Key modifier : code = %016x\n", keycode);
                
            if (LastShift != NewShift) {if (LastShift) mainWin->KeyUp(200); else mainWin->KeyDown(200);}
            if (LastCtrl  != NewCtrl)  {if (LastCtrl)  mainWin->KeyUp(201); else mainWin->KeyDown(201);}
            if (LastCaps  != NewCaps)  {if (LastCaps)  mainWin->KeyUp(202); else mainWin->KeyDown(202);}
            
            ctrl = keycode;
            break;
    }
}



extern "C" void beeb_MainCpuLoop()
{
#if 0 //ACH- iswindowcollapsed
    if ( (mainWin->m_FreezeWhenInactive) && (IsWindowCollapsed(mainWin->mWindow)) )
    {
        beeb_usleep(1000 * 500);        // sleep for 0.5 secs
    }
    else
#endif
    {
        int c;
        c = 20;

    // Menu GUI more responsive if running less than real time
        
        if ( (mainWin->m_RealTimeTarget != 0) && (mainWin->m_RealTimeTarget < 1) )
        {
            c = c * mainWin->m_RealTimeTarget;
        }
        
        for (int i = 0; i < c; ++i)
            Exec6502Instruction();
    }
}



int y = 0;
const int width = 640;
const int height = 512;


const int bpp = 4; // 4 bytes or 32 bits
unsigned char videobuffer[bpp * width * height]; // 200x200 by 4 bytes

extern "C" void beeb_video(int height, int width, struct CColour buffer[])
{
    for (int i = 0; i < width*height; i++)
    {
        buffer[i].b = videobuffer[0+(i*bpp)];
        buffer[i].g = videobuffer[1+(i*bpp)];
        buffer[i].r = videobuffer[2+(i*bpp)];
        buffer[i].a = videobuffer[3+(i*bpp)];
    }
    // move a green line everytime this is updated (50fps)
    y += 1;
    y %= height-2;
}


extern "C" int beeb_main(int argc,char *argv[])
{
//void *token;
int i;

    // NEED TO TURN OFF SANDBOXING IN ENTITLEMENTS FILE TO GET LOCAL FOLDERS TO WORK
    
  fprintf(stderr, "Version: %s %s\n", Version, VersionDate);
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }


  for (i = 0; i < argc; ++i)
    fprintf(stderr, "Arg %d = %s\n", i, argv[i]);

  mainWin=new BeebWin();
#if 0 //ACH - main init hotkey

  atexit(AtExitHandler);

  if (PushSymbolicHotKeyMode != NULL)
  {
    token = PushSymbolicHotKeyMode(kHIHotKeyModeAllDisabled);
  }


  tlog = fopen("/users/jonwelch/trace.log", "wt");
//  tlog = NULL;
#endif
    
  done = 0;
  
  mainWin->Initialise(argv[0]);

//  SoundReset();
  if (SoundEnabled) SoundInit();
  if (SpeechDefault) tms5220_start();
  mainWin->ResetBeebSystem(MachineType,TubeEnabled,1);
  mainWin->SetRomMenu();
  mainWin->SetSoundMenu();
#if 0 //ACH - main init events

  EventTypeSpec    eventTypes[7];
  EventHandlerUPP  handlerUPP;

  eventTypes[0].eventClass = kEventClassKeyboard;
  eventTypes[0].eventKind  = 1; // kEventRawKeyDown
  eventTypes[1].eventClass = kEventClassKeyboard;
  eventTypes[1].eventKind  = 3; // kEventRawKeyUp
  eventTypes[2].eventClass = kEventClassKeyboard;
  eventTypes[2].eventKind  = 4; // kEventRawKeyModifiersChanged

  eventTypes[3].eventClass = kEventClassMouse;
  eventTypes[3].eventKind  = kEventMouseDown;
  eventTypes[4].eventClass = kEventClassMouse;
  eventTypes[4].eventKind  = kEventMouseUp;
  eventTypes[5].eventClass = kEventClassMouse;
  eventTypes[5].eventKind  = kEventMouseMoved;
  eventTypes[6].eventClass = kEventClassMouse;
  eventTypes[6].eventKind  = kEventMouseDragged;

  handlerUPP = NewEventHandlerUPP(EventHandler);
  InstallApplicationEventHandler (handlerUPP,
                                7, eventTypes,
                                NULL, NULL);

  AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, NewAEEventHandlerUPP(AEodoc), 0, false);

  EventTypeSpec events[] = {
        { kEventClassWindow, kEventWindowClosed }
  };

  EventTypeSpec commands[] = {
        { kEventClassCommand, kEventCommandProcess }
  };

  InstallWindowEventHandler (mainWin->mWindow,
           NewEventHandlerUPP (MainWindowEventHandler),
           GetEventTypeCount(events), events,
           mainWin->mWindow, NULL);

  InstallWindowEventHandler(mainWin->mWindow,
            NewEventHandlerUPP (MainWindowCommandHandler),
            GetEventTypeCount(commands), commands,
            mainWin->mWindow, NULL);


//  mainWin->StartRecordingVideo("jon.mov", nil);
  
  // Call the event loop
  RunApplicationEventLoopWithCooperativeThreadSupport();
#endif
    
    // autorun if a filepath was set
    AutoRunFromPath();

    return(0);
}


extern "C" int beeb_end()
{
  fprintf(stderr, "Shutting Down ...\n");
  
  if (tlog) fclose(tlog);

  SoundReset();

#if 0 //ACH - symbolic hotkey
  if (PopSymbolicHotKeyMode != NULL)
  {
     PopSymbolicHotKeyMode(token);
  }
#endif

  return(0);
} /* main */



extern "C" void beeb_TapeControlOpenDialog()
{
    TapeControlOpenDialog();
}


extern "C" void beeb_TapeControlCloseDialog()
{
TapeControlCloseDialog();
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
//    char* propertyCHR = (char*)&property;

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
