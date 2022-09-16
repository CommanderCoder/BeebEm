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

int done = 0;
extern OSStatus TCWindowCommandHandler(UInt32 cmdID);
extern FILE *tlog;
extern unsigned char UEFOpen;

struct CColour{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

extern int __argc;
extern char** __argv;

extern OSStatus UKWindowCommandHandler(UInt32 cmdID);

extern BeebWin *mainWin;

#ifdef BEEBWIN
extern char AutoRunPath[];
extern void AutoRunFromPath();
#endif

extern "C" void beeb_autorun(char* path)
{
#ifdef BEEBWIN
    strcpy(AutoRunPath, path);
#endif
}

extern "C" void beeb_handlekeys(long eventkind, unsigned int keycode, char charCode)
{
     static int ctrl = 0x0000;
     int LastShift, LastCtrl, LastCaps, LastCmd;
     int NewShift, NewCtrl=0, NewCaps;
     static int NewCmd = 0; //remember CMD pressed
     
#ifdef BEEBWIN

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
//                    fprintf(stderr, "cmd-T pressed, NewCmd = %d\n", NewCmd);
                    trace_186 = 1 - trace_186;
                }
              else
#endif
                if (! (NewCmd) )
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
                LastCmd = ctrl & 0x100018; // capture left and right CMD

                NewShift = keycode & 0x20006; // capture left and right shift
                NewCtrl = keycode & 0x40001; // capture left
                NewCaps = keycode & 0x80020; // capture left ALT/Option
                NewCmd = keycode &  0x100018; // capture left and right CMD

            
//                fprintf(stderr, "Key modifier : code = %016x\n", keycode);
                
            if (LastShift != NewShift) {if (LastShift) mainWin->KeyUp(200); else mainWin->KeyDown(200);}
            if (LastCtrl  != NewCtrl)  {if (LastCtrl)  mainWin->KeyUp(201); else mainWin->KeyDown(201);}
            if (LastCaps  != NewCaps)  {if (LastCaps)  mainWin->KeyUp(202); else mainWin->KeyDown(202);}
            
            ctrl = keycode;
            break;
    }
#endif
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

// MAIN EQUIVALENT IS
// beeb_main
// beeb_MainCpuLoop
// beeb_end
//


extern "C" int beeb_main(long argc, char* argv[])
{
    // NEED TO TURN OFF SANDBOXING IN ENTITLEMENTS FILE TO GET LOCAL FOLDERS TO WORK
    // DON'T TURN OFF SANDBOXING - THIS IS UNSAFE - ALLOW USER FOLDERS

    __argc = argc;
    __argv = argv;
    
    
// show the path
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
    
    return (  mainInit() );
}



extern "C" void beeb_MainCpuLoop()
{
    int c = 20;

    // Menu GUI more responsive if running less than real time
    
    if ( (mainWin->m_RealTimeTarget != 0) && (mainWin->m_RealTimeTarget < 1) )
    {
        c = c * mainWin->m_RealTimeTarget;
    }
    
    for (int i = 0; i < c; ++i)
        mainStep();
}




extern "C" int beeb_end()
{
  fprintf(stderr, "Shutting Down ...\n");
  
    mainEnd();
  return(0);
} /* main */



extern "C" void beeb_TapeControlOpenDialog()
{
#ifdef BEEBWIN
    TapeControlOpenDialog();
#endif
}


extern "C" void beeb_TapeControlCloseDialog()
{
#ifdef BEEBWIN
    TapeControlCloseDialog();
#endif
}


extern "C" long beeb_TCHandleCommand(unsigned int cmdID)
{
#ifdef BEEBWIN
    char* cmdCHR = (char*)&cmdID;
    printf("%c%c%c%c", cmdCHR[3], cmdCHR[2], cmdCHR[1], cmdCHR[0]);
    return TCWindowCommandHandler(cmdID);
#else
    return 0L;
#endif
}
    
// user keyboard
extern "C" long beeb_UKHandleCommand(unsigned int cmdID)
{
#ifdef BEEBWIN

    char* cmdCHR = (char*)&cmdID;
    printf("%c%c%c%c", cmdCHR[3], cmdCHR[2], cmdCHR[1], cmdCHR[0]);
    return UKWindowCommandHandler(cmdID);
#else
    return 0L;
#endif

}

extern "C" long beeb_getTableRowsCount(const char* tablename)
{
#ifdef BEEBWIN

    if (UEFOpen)
        return map_lines;
#endif
    return 0;


}

char temp[256];

extern "C" const char* beeb_getTableCellData(UInt32 property, long itemID)
{
//    char* propertyCHR = (char*)&property;

//    printf("%c%c%c%c data %ld", propertyCHR[3], propertyCHR[2], propertyCHR[1], propertyCHR[0], itemID);
    
#ifdef BEEBWIN

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
#endif
    return temp;
}



extern "C" void beeb_ukhandlekeys(long eventkind, unsigned int keycode, char charCode)
{
#ifdef BEEBWIN
  switch (eventkind)
  {
      case kEventRawKeyDown:

          fprintf(stderr, "Key pressed: code = %d, '%c'\n", keycode, charCode);
          if (LastButton != 0)
          {
              SetBBCKeyForVKEY(keycode);
              ShowKey(LastButton);
          }
  }
#endif

}


extern "C" int beeb_HandleCommand(unsigned int cmdID)
{
#ifdef BEEBWIN
    char* cmdCHR = (char*)&cmdID;
    printf("HANDLECMD %c%c%c%c", cmdCHR[3], cmdCHR[2], cmdCHR[1], cmdCHR[0]);
    return mainWin->HandleCommand(cmdID);
#else
    return 0;
#endif
    
}


extern "C" void WriteLog(const char *fmt, ...)
{
}
