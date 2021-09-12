void MyDrawIntoPDFPage(CGContextRef pdfContext, PMRect pageRect, int starty, int nlines);
//static OSStatus MyCreatePageFormat(PMPrintSession printSession, PMPageFormat *pageFormat);
//static OSStatus DoPageSetupDialog(PMPrintSession printSession, PMPageFormat* pageFormat, Handle* flattendedPageFormat);
//static OSStatus	DetermineNumberOfPagesInDoc(PMPageFormat pageFormat, UInt32* numPages);
//static OSStatus DoPrintLoop(PMPrintSession printSession, PMPageFormat pageFormat, PMPrintSettings printSettings, int starty, int nlines);
//static OSStatus DoPrintDialog(PMPrintSession printSession, PMPageFormat pageFormat, PMPrintSettings* printSettings);
void ProcessPrintCommand(int starty, int nlines);
void DoPageSetup(void);

extern PMPageFormat pageFormat;
extern PMPrintSettings printSettings;
extern Handle flattenedPageFormat;
