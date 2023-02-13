/*
  File:    VideoFrameToGWorld.h
  
  Description: VideoFrameToGWorld is an example of decompressing frames from a video track
         into an offscreen buffer so the pixels can be manipulated at a later time.

  Author:    era

  Copyright:   � Copyright 2000 Apple Computer, Inc. All rights reserved.
  
  Disclaimer:  IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
        ("Apple") in consideration of your agreement to the following terms, and your
        use, installation, modification or redistribution of this Apple software
        constitutes acceptance of these terms.  If you do not agree with these terms,
        please do not use, install, modify or redistribute this Apple software.

        In consideration of your agreement to abide by the following terms, and subject
        to these terms, Apple grants you a personal, non-exclusive license, under Apple's
        copyrights in this original Apple software (the "Apple Software"), to use,
        reproduce, modify and redistribute the Apple Software, with or without
        modifications, in source and/or binary forms; provided that if you redistribute
        the Apple Software in its entirety and without modifications, you must retain
        this notice and the following text and disclaimers in all such redistributions of
        the Apple Software.  Neither the name, trademarks, service marks or logos of
        Apple Computer, Inc. may be used to endorse or promote products derived from the
        Apple Software without specific prior written permission from Apple.  Except as
        expressly stated in this notice, no other rights or licenses, express or implied,
        are granted by Apple herein, including but not limited to any patent rights that
        may be infringed by your derivative works or by other works in which the Apple
        Software may be incorporated.

        The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
        WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
        WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
        PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
        COMBINATION WITH YOUR PRODUCTS.

        IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
        CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
        GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
        ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
        OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
        (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
        ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
        
  Change History (most recent first): <1> 4/1/00 initial release

*/

#ifndef __VIDEO_FRAME_TO_GWORLD__
  #define __VIDEO_FRAME_TO_GWORLD__

#ifndef TARGET_API_MAC_CARBON
  #define TARGET_API_MAC_CARBON 1
#endif

// Typedefs
//------------------------------------------------------------------------------
typedef const OSTypePtr TypeListPtr;

#if TARGET_OS_MAC
  typedef MenuHandle      MenuReference;
  typedef WindowPtr      WindowReference;
  typedef NavObjectFilterUPP  QTFrameFileFilterUPP;
#endif


// Defines & Constants
//------------------------------------------------------------------------------
#define  BailNULL(n)  if (!n) goto bail;
#define  BailError(n) if (n) goto bail;
#if TARGET_OS_MAC
  #define PASCAL_RTN pascal
#endif

// Prototypes
//------------------------------------------------------------------------------
OSErr GetOneFileWithPreview (char *path, NavObjectFilterUPP myFilterProc);
OSErr SaveFile(const char *path, FSSpec *fs);
OSErr SaveFileMov(char *path, FSSpec *fs);
PASCAL_RTN void HandleNavEvent(NavEventCallbackMessage theCallBackSelector, NavCBRecPtr theCallBackParms, void *theCallBackUD);

unsigned pascal char DiscFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode);
unsigned pascal char UEFFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode);
unsigned pascal char KeyboardFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode);
unsigned pascal char IFDFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode);

#endif // __VIDEO_FRAME_TO_GWORLD__
