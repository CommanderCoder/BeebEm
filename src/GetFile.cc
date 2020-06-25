/*
  File:    GetFile.c
  
  Description: VideoFrameToGWorld is an example of decompressing frames from a video track
         into an offscreen buffer so the pixels can be manipulated at a later time.

  Author:    based on code from QTShell sample

  Copyright:   © Copyright 2000 Apple Computer, Inc. All rights reserved.
  
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

#include <string.h>
#include <unistd.h>
#include "GetFile.h"

//////////
//
// GetOneFileWithPreview
// Display the appropriate file-opening dialog box, with an optional QuickTime preview pane. If the user
// selects a file, return information about it using the theFSSpecPtr parameter.
//
// Note that both StandardGetFilePreview and NavGetFile use the function specified by theFilterProc as a
// file filter. This framework always passes NULL in the theFilterProc parameter. If you use this function
// in your own code, keep in mind that on Windows the function specifier must be of type FileFilterUPP and 
// on Macintosh it must be of type NavObjectFilterUPP. (You can use the QTFrame_GetFileFilterUPP to create
// a function specifier of the appropriate type.) Also keep in mind that Navigation Services expects a file 
// filter function to return true if a file is to be displayed, while the Standard File Package expects the
// filter to return false if a file is to be displayed.
//
//////////
OSErr GetOneFileWithPreview (char *path, NavObjectFilterUPP myFilterProc)
{
  NavReplyRecord    myReply;
  NavDialogOptions  myDialogOptions;
  NavEventUPP       myEventUPP = NewNavEventUPP(HandleNavEvent);
  OSErr				myErr = noErr;
  
  // specify the options for the dialog box
  NavGetDefaultDialogOptions(&myDialogOptions);
  myDialogOptions.dialogOptionFlags -= kNavNoTypePopup;
  myDialogOptions.dialogOptionFlags -= kNavAllowMultipleFiles;
  
  // prompt the user for a file

  myErr = NavGetFile(NULL, &myReply, &myDialogOptions, myEventUPP, NULL, myFilterProc, NULL, NULL);

  if ((myErr == noErr) && myReply.validRecord) {
    AEKeyword    myKeyword;
    DescType    myActualType;
    Size      myActualSize = 0;
	FSRef       theFSRef;
    
    // get the FSSpec for the selected file

    myErr = AEGetNthPtr(&(myReply.selection), 1, typeFSRef, &myKeyword, &myActualType, &theFSRef, sizeof(theFSRef), &myActualSize);
    if (myErr == noErr) 
	{
		UInt8 *ptr;
		ptr = (UInt8 *) path;
		FSRefMakePath(&theFSRef, ptr, 255);
		fprintf(stderr, "Picked %s\n", path);
      }
		
    NavDisposeReply(&myReply);
  }
  
  DisposeNavEventUPP(myEventUPP);
 
  return(myErr);
}

//////////
//
// HandleNavEvent
// A callback procedure that handles events while a Navigation Service dialog box is displayed.
//
//////////
PASCAL_RTN void HandleNavEvent(NavEventCallbackMessage theCallBackSelector, NavCBRecPtr theCallBackParms, void *theCallBackUD)
{
#pragma unused(theCallBackUD)
  
  if (theCallBackSelector == kNavCBEvent) {
    switch (theCallBackParms->eventData.eventDataParms.event->what) {
      case updateEvt:
#if TARGET_OS_MAC
        // Handle Update Event
#endif
        break;
      case nullEvent:
        // Handle Null Event
        break;
    }
  }
}

unsigned pascal char DiscFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode)
{
Boolean display = true;
NavFileOrFolderInfo *theInfo;
FSRef ref;
LSItemInfoRecord outInfo;

	theInfo = (NavFileOrFolderInfo *) info;

	if (theInfo->isFolder == true) return true;
	
	display = false;
	
	AECoerceDesc (theItem, typeFSRef, theItem);
	
	if (AEGetDescData(theItem, &ref, sizeof (FSRef)) == noErr)
	{
		outInfo.extension = NULL;
		
		if (LSCopyItemInfoForRef (&ref, kLSRequestExtension, &outInfo) == noErr)
		{
			if (outInfo.extension != NULL)
			{
				if (CFStringCompare(outInfo.extension, CFSTR("ssd"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("dsd"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("wdd"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("dos"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("adl"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("adf"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("img"), kCFCompareCaseInsensitive) == 0) display = true;
				CFRelease(outInfo.extension);
			}
		}
	}
	
	return display;
}

unsigned pascal char UEFFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode)
{
Boolean display = true;
NavFileOrFolderInfo *theInfo;
FSRef ref;
LSItemInfoRecord outInfo;

	theInfo = (NavFileOrFolderInfo *) info;

	if (theInfo->isFolder == true) return true;
	
	display = false;
	
	AECoerceDesc (theItem, typeFSRef, theItem);
	
	if (AEGetDescData(theItem, &ref, sizeof (FSRef)) == noErr)
	{
		outInfo.extension = NULL;
		
		if (LSCopyItemInfoForRef (&ref, kLSRequestExtension, &outInfo) == noErr)
		{
			if (outInfo.extension != NULL)
			{
				if (CFStringCompare(outInfo.extension, CFSTR("uef"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("csw"), kCFCompareCaseInsensitive) == 0) display = true;
				CFRelease(outInfo.extension);
			}
		}
	}
	
	return display;
}

unsigned pascal char IFDFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode)
{
	Boolean display = true;
	NavFileOrFolderInfo *theInfo;
	FSRef ref;
	LSItemInfoRecord outInfo;
	
	theInfo = (NavFileOrFolderInfo *) info;
	
	if (theInfo->isFolder == true) return true;
	
	return true;
	
	display = false;
	
	AECoerceDesc (theItem, typeFSRef, theItem);
	
	if (AEGetDescData(theItem, &ref, sizeof (FSRef)) == noErr)
	{
		outInfo.extension = NULL;
		
		if (LSCopyItemInfoForRef (&ref, kLSRequestExtension, &outInfo) == noErr)
		{
			if (outInfo.extension != NULL)
			{
				if (CFStringCompare(outInfo.extension, CFSTR("inf"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("ssd"), kCFCompareCaseInsensitive) == 0) display = true;
				if (CFStringCompare(outInfo.extension, CFSTR("dsd"), kCFCompareCaseInsensitive) == 0) display = true;
				CFRelease(outInfo.extension);
			}
		}
	}
	
	return display;
}


unsigned pascal char KeyboardFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes FilterMode)
{
	Boolean display = true;
	NavFileOrFolderInfo *theInfo;
	FSRef ref;
	LSItemInfoRecord outInfo;
	
	theInfo = (NavFileOrFolderInfo *) info;
	
	if (theInfo->isFolder == true) return true;
	
	display = false;
	
	AECoerceDesc (theItem, typeFSRef, theItem);
	
	if (AEGetDescData(theItem, &ref, sizeof (FSRef)) == noErr)
	{
		outInfo.extension = NULL;
		
		if (LSCopyItemInfoForRef (&ref, kLSRequestExtension, &outInfo) == noErr)
		{
			if (outInfo.extension != NULL)
			{
				if (CFStringCompare(outInfo.extension, CFSTR("kmap"), kCFCompareCaseInsensitive) == 0) display = true;
				CFRelease(outInfo.extension);
			}
		}
	}
	
	return display;
}

OSErr SaveFile(char *path, FSSpec *fs) 
{
NavReplyRecord    myReply;
NavDialogOptions    myDialogOptions;
NavEventUPP    myEventUPP = NULL;
OSErr        myErr = noErr;
OSType	fileTypeToSave = 'TEXT', fileCreator = 'jjf0';

  // specify the options for the dialog box
  NavGetDefaultDialogOptions(&myDialogOptions);
  myDialogOptions.dialogOptionFlags += kNavNoTypePopup;
  myDialogOptions.dialogOptionFlags += kNavDontAutoTranslate;
  
  // prompt the user for a file
  myErr = NavPutFile(NULL, &myReply, &myDialogOptions, myEventUPP, fileTypeToSave, fileCreator, NULL);

  if ((myErr == noErr) && myReply.validRecord) {
    AEKeyword    myKeyword;
    DescType    myActualType;
    Size      myActualSize = 0;
	FSSpec		theFSSpec;
	FSRef       theFSRef;
    
    myErr = AEGetNthPtr(&(myReply.selection), 1, typeFSS, &myKeyword, &myActualType, &theFSSpec, sizeof(FSSpec), &myActualSize);

    if ((myErr == noErr) && myReply.validRecord)
	{
		myErr = FSpCreate(&theFSSpec, fileCreator, fileTypeToSave, smSystemScript);
		myErr = FSpMakeFSRef(&theFSSpec, &theFSRef);
		UInt8 *ptr;
		ptr = (UInt8 *) path;
		FSRefMakePath(&theFSRef, ptr, 255);
		fprintf(stderr, "Picked %s\n", path);
		unlink(path);
		if (fs) memcpy(fs, &theFSSpec, sizeof(FSSpec));
	}
		
    NavDisposeReply(&myReply);
  }

  DisposeNavEventUPP(myEventUPP);

  return (myErr);
}

OSErr SaveFileMov(char *path, FSSpec *fs) 
{
	NavReplyRecord    myReply;
	NavDialogOptions    myDialogOptions;
	NavEventUPP    myEventUPP = NULL;
	OSErr        myErr = noErr;
	OSType	fileTypeToSave = 'MooV', fileCreator = 'TVOD';
	
	// specify the options for the dialog box
	NavGetDefaultDialogOptions(&myDialogOptions);
	myDialogOptions.dialogOptionFlags += kNavNoTypePopup;
	myDialogOptions.dialogOptionFlags += kNavDontAutoTranslate;
	
	// prompt the user for a file
	myErr = NavPutFile(NULL, &myReply, &myDialogOptions, myEventUPP, fileTypeToSave, fileCreator, NULL);
	
	if ((myErr == noErr) && myReply.validRecord) {
		AEKeyword    myKeyword;
		DescType    myActualType;
		Size      myActualSize = 0;
		FSSpec		theFSSpec;
		FSRef       theFSRef;
		
		myErr = AEGetNthPtr(&(myReply.selection), 1, typeFSS, &myKeyword, &myActualType, &theFSSpec, sizeof(FSSpec), &myActualSize);
		
		if ((myErr == noErr) && myReply.validRecord)
		{
			myErr = FSpCreate(&theFSSpec, fileCreator, fileTypeToSave, smSystemScript);
			myErr = FSpMakeFSRef(&theFSSpec, &theFSRef);
			UInt8 *ptr;
			ptr = (UInt8 *) path;
			FSRefMakePath(&theFSRef, ptr, 255);
			fprintf(stderr, "Picked %s\n", path);
			unlink(path);
			if (fs) memcpy(fs, &theFSSpec, sizeof(FSSpec));
		}
		
		NavDisposeReply(&myReply);
	}
	
	DisposeNavEventUPP(myEventUPP);
	
	return (myErr);
}
