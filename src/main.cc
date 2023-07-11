/****************************************************************************/
/*              Beebem - (c) David Alan Gilbert 1994/1995                   */
/*              -----------------------------------------                   */
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
/* Beeb emulator - main file */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <Carbon/Carbon.h>

#include "model.h"

#include "6502core.h"
#include "beebmem.h"
#include "beebsound.h"
#include "sysvia.h"
#include "uservia.h"
#include "beebwin.h"
#include "disc8271.h"
#include "tube.h"
#include "video.h"
#include "defines.h"
#include "tube.h"
#include "z80mem.h"
#include "z80.h"
#include "speech.h"
#include "main.h"


#if 0 //ACH  -push/pop symbolic hotkey

extern void * PushSymbolicHotKeyMode(OptionBits inOptions) __attribute__((weak_import));
extern void PopSymbolicHotKeyMode(void * inToken)          __attribute__((weak_import));
#endif

FILE *tlog;
int trace;
int DumpAfterEach=0;
#include "via.h"
extern VIAState SysVIAState;
BeebWin *mainWin;
Model MachineType;
Boolean quitNow;

const char *Version="5.0a";
const char *VersionDate="25th June 2020";

#if 0 //ACH - auto run

void AtExitHandler(void) {
  delete mainWin;
}; /* AtExitHandler */

static pascal OSErr AEodoc(const AppleEvent *theEvent, AppleEvent *theReply, long refCon)
{
	OSErr 		err;
	FSRef		ref;
	AEDescList	docList;
	AEKeyword	keywd;

	DescType	rtype;
	Size		acsize;
	long		count;
	
	#pragma unused (theReply, refCon)
	
//	if (running)
//		return noErr;

	err = AEGetParamDesc(theEvent, keyDirectObject, typeAEList, &docList);
	if (err)
		return noErr;
	
	err = AECountItems(&docList, &count);
	if (err || (count != 1))
	{
		err = AEDisposeDesc(&docList);
		return noErr;
	}
	
	err = AEGetNthPtr(&docList, 1, typeFSRef, &keywd, &rtype, &ref, sizeof(FSRef), &acsize);
	if (err == noErr)
	{
		char path[256];
		UInt8 *ptr;
		ptr = (UInt8 *) path;
		FSRefMakePath(&ref, ptr, 255);
		fprintf(stderr, "Autoran with %s\n", path);
		if (mainWin)
		{
			if (strstr(path, ".uef")) mainWin->LoadTapeFromPath(path);
			else if (strstr(path, ".UEF")) mainWin->LoadTapeFromPath(path);
			else if (strstr(path, ".csw")) mainWin->LoadTapeFromPath(path);
			else if (strstr(path, ".CSW")) mainWin->LoadTapeFromPath(path);
			else {
				mainWin->LoadDisc(0, path);
				mainWin->m_ShiftBooted = true;
				mainWin->ResetBeebSystem(MachineType, TubeEnabled, 0);
				BeebKeyDown(0, 0);
				
			}
		}
    }
		
	err = AEDisposeDesc(&docList);

	return err;
}

#else

char AutoRunPath[512]="";

void AutoRunFromPath()
{
    char* path = AutoRunPath;
    if (mainWin && path[0] != 0)
    {
        if (strstr(path, ".uef")) mainWin->LoadTapeFromPath(path);
        else if (strstr(path, ".UEF")) mainWin->LoadTapeFromPath(path);
        else if (strstr(path, ".csw")) mainWin->LoadTapeFromPath(path);
        else if (strstr(path, ".CSW")) mainWin->LoadTapeFromPath(path);
        else {
            mainWin->LoadDisc(0, path);
            mainWin->m_ShiftBooted = true;
            mainWin->ResetBeebSystem(MachineType, TubeEnabled, 0);
            BeebKeyDown(0, 0);
            
        }
        // remove the path, ready for the next autorun
        path[0] = 0;
    }

}


#endif

#if 0 // ACH - Event Handler and Main Loop

static OSStatus MainWindowEventHandler(
   EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    OSStatus err = noErr;

	switch (GetEventKind(event))
    {
        case kEventWindowClosed: 
			quitNow = true;
            QuitApplicationEventLoop();
            break;
        default:
            err = eventNotHandledErr;
            break;
    }
    
    return err;
}

static OSStatus MainWindowCommandHandler(
    EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    HICommand command; 
//    WindowRef window = (WindowRef) userData;
    OSStatus err = noErr;
    err = GetEventParameter(event, kEventParamDirectObject,
        typeHICommand, NULL, sizeof(HICommand), NULL, &command);
    require_noerr (err, CantGetParameter);

	err = mainWin->HandleCommand(command.commandID);
    
CantGetParameter:
    return err;
}
#endif

#if 0//ach - eventhandlers / keys&mouse, quit, main loop

static OSStatus EventHandler (
    EventHandlerCallRef handler, EventRef event, void *data)
{
char charCode;
int keycode;
static int ctrl = 0x0000;
int LastShift, LastCtrl, LastCaps, LastCmd;
int NewShift, NewCtrl, NewCaps;
static int NewCmd = 0;

  switch (GetEventClass(event)) 
  {
  case kEventClassKeyboard:
    
//	fprintf(stderr, "Key Event Kind %d\n", GetEventKind(event));
	
    switch (GetEventKind(event)) 
	{
      case kEventRawKeyDown:
        GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
        GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(int), NULL, &keycode);
//      fprintf(stderr, "Key pressed: code = %d, '%c'\n", keycode, charCode);
		if ( (NewCmd) && (keycode == 6) )
		{
//			fprintf(stderr, "cmd-Z pressed, NewCmd = %d\n", NewCmd);
			if (mainWin->m_isFullScreen)
			{
				EndFullScreen(mainWin->m_RestoreState, nil);
				mainWin->m_isFullScreen = 0;
				mainWin->SetMenuCommandIDCheck('vfsc', false);
			}
		} else if ( (NewCmd) && (NewCtrl) && (keycode == 1) )
		{
//			fprintf(stderr, "ctrl-cmd-S pressed, NewCmd = %d\n", NewCmd);
			mainWin->m_PrintScreen = true;
		} else if ( (NewCmd) && (keycode == 8) )
		{
//			fprintf(stderr, "cmd-C pressed, NewCmd = %d\n", NewCmd);
			mainWin->doCopy();
		} else if ( (NewCmd) && (keycode == 9) )
		{
//			fprintf(stderr, "cmd-V pressed, NewCmd = %d\n", NewCmd);
			mainWin->doPaste();
		} else if ( (NewCmd) && (keycode == 17) )
		{
			trace_186 = 1 - trace_186;
		}
	  else
		{
			mainWin->KeyDown(keycode);
		}
		break;
      case kEventRawKeyUp:
        GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
        GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(int), NULL, &keycode);
//        fprintf(stderr, "Key released: code = %d, '%c'\n", keycode, charCode);
		mainWin->KeyUp(keycode);
		break;
      case 4:
        GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(int), NULL, &keycode);
	    LastCmd = ctrl & 0x0100;
		LastShift = ctrl & 0x0200;
		LastCtrl = ctrl & 0x1000;
		LastCaps = ctrl & 0x0800;
		NewShift = keycode & 0x0200;
		NewCtrl = keycode & 0x1000;
		NewCaps = keycode & 0x0800;
	    NewCmd = keycode & 0x0100;

//		fprintf(stderr, "Key modifier : code = %08x\n", keycode);
		
		if (LastShift != NewShift) if (LastShift) mainWin->KeyUp(200); else mainWin->KeyDown(200);
		if (LastCtrl  != NewCtrl)  if (LastCtrl)  mainWin->KeyUp(201); else mainWin->KeyDown(201);
		if (LastCaps  != NewCaps)  if (LastCaps)  mainWin->KeyUp(202); else mainWin->KeyDown(202);
		ctrl = keycode;
      break;
    }
    break;

  case kEventClassMouse:

//	WriteLog("Key Event Kind %d\n", GetEventKind(event));
	
	Point posn;
	EventMouseButton btn;
	HIPoint wposn;

    switch (GetEventKind(event)) 
	{
      case kEventMouseDown:
        GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &posn);
        GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &btn);
        GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &wposn);
//        WriteLog("Mouse Down : Screen X = %d, Screen Y = %d, Button = %d, Window X = %f, Window Y = %f\n", posn.h, posn.v, btn, wposn.x, wposn.y);
		if ( (wposn.x > 0) && (wposn.y > 0) )
		{
			switch (btn)
			{
			case kEventMouseButtonPrimary:
				mainWin->SetMousestickButton(0,TRUE);
				AMXButtons |= AMX_LEFT_BUTTON;
				break;
			case kEventMouseButtonSecondary:
                mainWin->SetMousestickButton(1,TRUE);
				AMXButtons |= AMX_RIGHT_BUTTON;
				break;
			case kEventMouseButtonTertiary:
				AMXButtons |= AMX_MIDDLE_BUTTON;
				break;
			}
		}
		break;
      case kEventMouseUp:
        GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &posn);
        GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &btn);
        GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &wposn);
//        WriteLog("Mouse Up : Screen X = %d, Screen Y = %d, Button = %d, Window X = %f, Window Y = %f\n", posn.h, posn.v, btn, wposn.x, wposn.y);
		if ( (wposn.x > 0) && (wposn.y > 0) )
			switch (btn)
			{
			case kEventMouseButtonPrimary:
				mainWin->SetMousestickButton(FALSE);
				AMXButtons &= ~AMX_LEFT_BUTTON;
				break;
			case kEventMouseButtonSecondary:
				AMXButtons &= ~AMX_RIGHT_BUTTON;
				break;
			case kEventMouseButtonTertiary:
				AMXButtons &= ~AMX_MIDDLE_BUTTON;
				break;
			}
		break;
      case kEventMouseMoved:
        GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &posn);
        GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &wposn);
//        WriteLog("Mouse Moved : Screen X = %d, Screen Y = %d, Window X = %f, Window Y = %f\n", posn.h, posn.v, wposn.x, wposn.y);
			if ( (wposn.x > 0) && (wposn.y > 20) )
			{
				mainWin->ScaleMousestick( (int) wposn.x, (int) wposn.y - 21);
				mainWin->SetAMXPosition( (int) wposn.x, (int) wposn.y - 21);		// take off height of menu bar
			}
      break;
      case kEventMouseDragged:
		  GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &posn);
		  GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &wposn);
//          WriteLog("Mouse Dragged : Screen X = %d, Screen Y = %d, Window X = %f, Window Y = %f\n", posn.h, posn.v, wposn.x, wposn.y);
		  if ( (wposn.x > 0) && (wposn.y > 20) )
		  {
			  mainWin->ScaleMousestick( (int) wposn.x, (int) wposn.y - 21);
			  mainWin->SetAMXPosition( (int) wposn.x, (int) wposn.y - 21);		// take off height of menu bar
		  }
	  break;
    }

	break;
  }

  return eventNotHandledErr;
  
}

extern SInt32 gNumberOfRunningThreads;
    // This variable must be maintained by your thread scheduling
    // code to accurately reflect the number of threads that are
    // ready and need time for computation.

static EventHandlerUPP gQuitEventHandlerUPP;   // -> QuitEventHandler

static pascal OSStatus QuitEventHandler(EventHandlerCallRef inHandlerCallRef,
                                        EventRef inEvent, void *inUserData)
    // This event handler is used to override the kEventClassApplication/
    // kEventAppQuit event while inside our event loop (EventLoopEventHandler).
    // It simply calls through to the next handler and, if that handler returns
    // noErr (indicating that the application is doing to quit), it sets
    // a Boolean to tell our event loop to quit as well.
{
    OSStatus err;

    err = CallNextEventHandler(inHandlerCallRef, inEvent);
    if (err == noErr) {
        *((Boolean *) inUserData) = true;
    }

    return err;
}

static EventHandlerUPP gEventLoopEventHandlerUPP;   // -> EventLoopEventHandler

static pascal OSStatus EventLoopEventHandler(EventHandlerCallRef inHandlerCallRef,
                                             EventRef inEvent, void *inUserData)
    // This code contains the standard Carbon event dispatch loop,
    // as per "Inside Macintosh: Handling Carbon Events", Listing 3-10,
    // except:
    //
    // o this loop supports yielding to cooperative threads based on the
    //   application maintaining the gNumberOfRunningThreads global
    //   variable, and
    //
    // o it also works around a problem with the Inside Macintosh code
    //   which unexpectedly quits when run on traditional Mac OS 9.
    //
    // See RunApplicationEventLoopWithCooperativeThreadSupport for
    // an explanation of why this is inside a Carbon event handler.
    //
    // The code in Inside Mac has a problem in that it quits the
    // event loop when ReceiveNextEvent returns an error.  This is
    // wrong because ReceiveNextEvent can return eventLoopQuitErr
    // when you call WakeUpProcess on traditional Mac OS.  So, rather
    // than relying on an error from ReceiveNextEvent, this routine tracks
    // whether the application is really quitting by installing a
    // customer handler for the kEventClassApplication/kEventAppQuit
    // Carbon event.  All the custom handler does is call through
    // to the previous handler and, if it returns noErr (which indicates
    // the application is quitting, it sets quitNow so that our event
    // loop quits.
    //
    // Note that this approach continues to support QuitApplicationEventLoop,
    // which is a simple wrapper that just posts a kEventClassApplication/
    // kEventAppQuit event to the event loop.
{
    OSStatus        err;
    OSStatus        junk;
    EventHandlerRef installedHandler;
    EventTargetRef  theTarget;
    EventRef        theEvent;
    EventTimeout    timeToWaitForEvent;
	
    static const EventTypeSpec eventSpec = {kEventClassApplication, kEventAppQuit};

    quitNow = false;

    // Install our override on the kEventClassApplication, kEventAppQuit event.

    err = InstallEventHandler(GetApplicationEventTarget(), gQuitEventHandlerUPP,
                              1, &eventSpec, &quitNow, &installedHandler);

	if (err == noErr) {

        // Run our event loop until quitNow is set.

        theTarget = GetEventDispatcherTarget();
        do {
            timeToWaitForEvent = kEventDurationNoWait;
            err = ReceiveNextEvent(0, NULL, timeToWaitForEvent,
                                   true, &theEvent);
            if (err == noErr) {
                (void) SendEventToEventTarget(theEvent, theTarget);
                ReleaseEvent(theEvent);
            }

			if ( (mainWin->m_FreezeWhenInactive) && (IsWindowCollapsed(mainWin->mWindow)) )
			{
				usleep(1000 * 500);		// sleep for 0.5 secs
			}
			else
			{
				int c;
				c = 20;

// Menu GUI more responsive if running less than real time
				
				if ( (mainWin->m_RealTimeTarget != 0) && (mainWin->m_RealTimeTarget < 1) )
				{
					c = c * mainWin->m_RealTimeTarget;
				}
				
				for (int i = 0; i < c; ++i) 
					if (!quitNow) Exec6502Instruction();
			}
			
//			Point p;
//			GetMouse(&p);
//			WriteLog("Mouse At : Window X = %d, Window Y = %d\n", p.h, p.v);
//			mainWin->ScaleMousestick( p.h, p.v);
//			mainWin->SetAMXPosition( p.h, p.v);

        } while ( ! quitNow );

        // Clean up.

		junk = RemoveEventHandler(installedHandler);

    }

    // So we can tell when our event loop quit.

//    SysBeep(10);

    return err;
}

static void RunApplicationEventLoopWithCooperativeThreadSupport(void)
    // A reimplementation of RunApplicationEventLoop that supports
    // yielding time to cooperative threads.  It relies on the
    // rest of your application to maintain a global variable,
    // gNumberOfRunningThreads, that reflects the number of threads
    // that are ready to run.
{
    static const EventTypeSpec eventSpec = {'KWIN', 'KWIN' };
    OSStatus        err;
    OSStatus        junk;
    EventHandlerRef installedHandler;
    EventRef        dummyEvent;

    dummyEvent = nil;

    // Create a UPP for EventLoopEventHandler and QuitEventHandler
    // (if we haven't already done so).

    err = noErr;
    if (gEventLoopEventHandlerUPP == nil) {
        gEventLoopEventHandlerUPP = NewEventHandlerUPP(EventLoopEventHandler);
    }
    if (gQuitEventHandlerUPP == nil) {
        gQuitEventHandlerUPP = NewEventHandlerUPP(QuitEventHandler);
    }
    if (gEventLoopEventHandlerUPP == nil || gQuitEventHandlerUPP == nil) {
        err = memFullErr;
    }

    // Install EventLoopEventHandler, create a dummy event and post it,
    // and then call RunApplicationEventLoop.  The rationale for this
    // is as follows:  We want to unravel RunApplicationEventLoop so
    // that we can can yield to cooperative threads.  In fact, the
    // core code for RunApplicationEventLoop is pretty easy (you
    // can see it above in EventLoopEventHandler).  However, if you
    // just execute this code you miss out on all the standard event
    // handlers.  These are relatively easy to reproduce (handling
    // the quit event and so on), but doing so is a pain because
    // a) it requires a bunch boilerplate code, and b) if Apple
    // extends the list of standard event handlers, your application
    // wouldn't benefit.  So, we execute our event loop from within
    // a Carbon event handler that we cause to be executed by
    // explicitly posting an event to our event loop.  Thus, the
    // standard event handlers are installed while our event loop runs.

    if (err == noErr) {
        err = InstallEventHandler(GetApplicationEventTarget(), gEventLoopEventHandlerUPP,
                                  1, &eventSpec, nil, &installedHandler);
        if (err == noErr) {
            err = MacCreateEvent(nil, 'KWIN', 'KWIN', GetCurrentEventTime(),
                                  kEventAttributeNone, &dummyEvent);
            if (err == noErr) {
                err = PostEventToQueue(GetMainEventQueue(), dummyEvent,
                                  kEventPriorityHigh);
            }
            if (err == noErr) {
                RunApplicationEventLoop();
            }

            junk = RemoveEventHandler(installedHandler);
        }
    }

    // Clean up.

    if (dummyEvent != nil) {
        ReleaseEvent(dummyEvent);
    }
}

#endif

void WriteLog(const char *fmt, ...)
{
char buff[512];
	
	va_list argptr;
	
	va_start(argptr, fmt);
	vsprintf(buff, fmt, argptr);
	va_end(argptr);

	if (tlog) fprintf(tlog, "%s", buff);
	fprintf(stderr, "%s", buff);
}
