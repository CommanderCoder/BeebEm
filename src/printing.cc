#include <Carbon/Carbon.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "main.h"
#include "printing.h"

//-----------------------------------------------------------------------------------------------------------------------
static OSStatus MyCreatePageFormat(PMPrintSession printSession, PMPageFormat *pageFormat)
{
        OSStatus status = PMCreatePageFormat(pageFormat);
            
        //  Note that PMPageFormat is not session-specific, but calling
        //  PMSessionDefaultPageFormat assigns values specific to the printer
        //  associated with the current printing session.
        if ((status == noErr) && (*pageFormat != kPMNoPageFormat))
            status = PMSessionDefaultPageFormat(printSession, *pageFormat);
	    
	return status;
}

//-----------------------------------------------------------------------------------------------------------------------
// (Borrowed from /Developer/Examples/Printing/App/)
static OSStatus DoPageSetupDialog(PMPrintSession printSession, PMPageFormat* pageFormat, Handle* flattendedPageFormat)
{
    OSStatus	status = noErr;
    
    if (*pageFormat == kPMNoPageFormat)    // Set up a valid PageFormat object
    {
		MyCreatePageFormat(printSession, pageFormat);
    }
    else
    {
        status = PMSessionValidatePageFormat(printSession, *pageFormat, kPMDontWantBoolean);
    }

    if (status == noErr)            //	Display the Page Setup dialog
    {
        Boolean accepted;
        status = PMSessionPageSetupDialog(printSession, *pageFormat, &accepted);
        if (status == noErr && !accepted)
            status = kPMCancel;		// user clicked Cancel button
    }	
                            
    //	If the user did not cancel, flatten and save the PageFormat object with our document
    if ((status == noErr) && (flattendedPageFormat != NULL))
    {
//        status = FlattenAndSavePageFormat(*pageFormat);
        status = PMFlattenPageFormat(*pageFormat, flattendedPageFormat);
    }
    
    return status;
} // DoPageSetupDialog

//-----------------------------------------------------------------------------------------------------------------------
static OSStatus	DetermineNumberOfPagesInDoc(PMPageFormat pageFormat, UInt32* numPages)
{
    PMRect	pageRect;
    OSStatus	status  = PMGetAdjustedPageRect(pageFormat, &pageRect);
    check(status == noErr);

    *numPages = 1;  // will do better some time in the future ...

    return status;
    
} // DetermineNumberOfPagesinDoc

//-----------------------------------------------------------------------------------------------------------------------
static OSStatus DoPrintLoop(PMPrintSession printSession, PMPageFormat pageFormat, PMPrintSettings printSettings, int starty, int nlines)
{
    OSStatus        status = noErr, tempErr;
    CGContextRef    printingCtx;
    UInt32          realNumberOfPagesinDoc,
                    pageNumber,
                    firstPage,
                    lastPage;
    CFStringRef     jobName;    // use window title

    status = CopyWindowTitleAsCFString(mainWin->mWindow, &jobName);

    status = PMSetJobNameCFString(printSettings, jobName);
    CFRelease (jobName);

    //	Get the user's Print dialog selection for first and last pages to print.
    if (status == noErr)
    {
        status = PMGetFirstPage(printSettings, &firstPage);
        if (status == noErr)
            status = PMGetLastPage(printSettings, &lastPage);
    }

    //	Check that the selected page range does not exceed the actual number of pages in the document.
    if (status == noErr)
    {
        status = DetermineNumberOfPagesInDoc(pageFormat, &realNumberOfPagesinDoc);
        if (realNumberOfPagesinDoc < lastPage)
            lastPage = realNumberOfPagesinDoc;
    }

    //	Before executing the print loop, tell the Carbon Printing Manager which pages
    //	will be spooled so that the progress dialog can reflect an accurate page count.
	
    if (status == noErr)
        status = PMSetFirstPage(printSettings, firstPage, false);
    if (status == noErr)
        status = PMSetLastPage(printSettings, lastPage, false);
    	
    //	Note, we don't have to worry about the number of copies.  The printing
    //	manager handles this.  So we just iterate through the document from the
    //	first page to be printed, to the last.
    
    // Now, tell the printing system that we promise never to use any Quickdraw calls:
    {
        CFStringRef s[1] = { kPMGraphicsContextCoreGraphics };
        CFArrayRef  graphicsContextsArray = CFArrayCreate(NULL, (const void**)s, 1, &kCFTypeArrayCallBacks);
        PMSessionSetDocumentFormatGeneration(printSession, kPMDocumentFormatPDF, graphicsContextsArray, NULL);
        CFRelease(graphicsContextsArray);
    }
    
    if (status == noErr)
    {
        status = PMSessionBeginDocument(printSession, printSettings, pageFormat);
        check(status == noErr);
        if (status == noErr)
        {
            pageNumber = firstPage;
        
            // Note that we check PMSessionError immediately before beginning a new page.
            // This handles user cancelling appropriately. Also, if we got an error on 
            // any previous iteration of the print loop, we break out of the loop.
            while ( (pageNumber <= lastPage) && (status == noErr) && (PMSessionError(printSession) == noErr) )
            {
                status = PMSessionBeginPage(printSession, pageFormat, NULL);

				check(status == noErr);
                if (status == noErr)
                {
                    status = PMSessionGetGraphicsContext(printSession, kPMGraphicsContextCoreGraphics, (void**)&printingCtx);
                    check(status == noErr);
                    if (status == noErr) 
                    {
						PMRect       pageRect;
						PMGetAdjustedPaperRect(pageFormat, &pageRect);

						MyDrawIntoPDFPage(printingCtx, pageRect, starty, nlines);
                    }
                                    
                    tempErr = PMSessionEndPage(printSession);
                    if(status == noErr)
                       status = tempErr;
					
                }
                pageNumber++;
            } // end while loop
                    
            // Close the print job.  This dismisses the progress dialog on Mac OS X.
            tempErr = PMSessionEndDocument(printSession);
            if (status == noErr)
                status = tempErr;
        }
    }
            
    //	Only report a printing error once we have completed the print loop. This ensures
    //	that every PMBeginXXX call that returns no error is followed by a matching PMEndXXX
    //  call, so the Printing Manager can release all temporary memory and close properly.
    tempErr = PMSessionError(printSession);
    if(status == noErr)
        status = tempErr;
/*
    if ((status != noErr) && (status != kPMCancel))
        PostPrintingErrors(status);
*/		
    return status;
} // DoPrintLoop


//-----------------------------------------------------------------------------------------------------------------------
// (Borrowed from /Developer/Examples/Printing/App/)
static OSStatus DoPrintDialog(PMPrintSession printSession, PMPageFormat pageFormat, PMPrintSettings* printSettings)
{
    OSStatus	status = noErr;
    Boolean     accepted;
    UInt32      realNumberOfPagesinDoc;
    
    //	In this sample code the caller provides a valid PageFormat reference but in
    //	your application you may want to load and unflatten the PageFormat object
    //	that was saved at PageSetup time.  See LoadAndUnflattenPageFormat below.
    
    //	Set up a valid PrintSettings object.
    if (*printSettings == kPMNoPrintSettings)
    {
        status = PMCreatePrintSettings(printSettings);	
        check(status == noErr);
        // Note that PMPrintSettings is not session-specific, but calling
        // PMSessionDefaultPrintSettings assigns values specific to the printer
        // associated with the current printing session.
        if ((status == noErr) && (*printSettings != kPMNoPrintSettings))
            status = PMSessionDefaultPrintSettings(printSession, *printSettings);
        check(status == noErr);
    }
    else
    {
        status = PMSessionValidatePrintSettings(printSession, *printSettings, kPMDontWantBoolean);
        check(status == noErr);
    }
    
    // Before displaying the Print dialog, we calculate the number of pages in the
    // document.  On Mac OS X this is useful because we can prime the Print dialog
    // with the actual page range of the document and prevent the user from entering
    // out-of-range numbers.  This is not possible on Mac OS 8 and 9 because the driver,
    // not the printing manager, controls the page range fields in the Print dialog.

    // Calculate the number of pages required to print the entire document.
    if (status == noErr)
        status = DetermineNumberOfPagesInDoc(pageFormat, &realNumberOfPagesinDoc);

    // Set a valid page range before displaying the Print dialog
    if (status == noErr)
        status = PMSetPageRange(*printSettings, 1, realNumberOfPagesinDoc);
    check(status == noErr);

    //	Display the Print dialog.
    if (status == noErr)
    {
        status = PMSessionPrintDialog(printSession, *printSettings, pageFormat, &accepted);
        check(status == noErr);
        if (status == noErr && !accepted)
            status = kPMCancel;		// user clicked Cancel button
    }

    return status;
	
}   // DoPrintDialog

PMPageFormat pageFormat = NULL;
PMPrintSettings printSettings = NULL;
Handle flattenedPageFormat;

//-----------------------------------------------------------------------------------------------------------------------
void ProcessPrintCommand(int starty, int nlines)
{
    PMPrintSession  printSession = NULL;
    
    if ( PMCreateSession(&printSession) == noErr )
    {

		if (pageFormat == NULL)
		{
			DoPageSetup();
		}
		
//		MyCreatePageFormat(printSession, &pageFormat);
	
		if (DoPrintDialog(printSession, pageFormat, &printSettings) == noErr)
		{
			DoPrintLoop(printSession, pageFormat, printSettings, starty, nlines);
		}
		PMRelease(printSession);
    }
    else
    {
		fprintf(stderr, "PMCreateSession FAILED\n");
    }
}   // ProcessPrintCommand

//-----------------------------------------------------------------------------------------------------------------------
void DoPageSetup(void)
{
    PMPrintSession  printSession = NULL;
    
    if ( PMCreateSession(&printSession) == noErr )
    {
		DoPageSetupDialog(printSession, &pageFormat, &flattenedPageFormat);
		PMRelease(printSession);
    }
    else
    {
		fprintf(stderr, "PMCreateSession FAILED\n");
    }
}   // ProcessPrintCommand


