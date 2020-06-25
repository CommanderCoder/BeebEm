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
/* The window which the beeb emulator displays stuff in */
/* David Alan Gilbert 6/11/94 */

#include <Carbon/Carbon.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "main.h"
#include "beebwin.h"
#include "port.h"
#include "6502core.h"
#include "disc8271.h"
#include "disc1770.h"
#include "sysvia.h"
#include "uservia.h"
#include "video.h"
#include "beebsound.h"
#include "beebmem.h"
#include "atodconv.h"
#include "serial.h"
#include "econet.h"
#include "tube.h"
#include "uefstate.h"
#include "debug.h"
#include "scsi.h"
#include "sasi.h"
#include "defines.h"
#include "GetFile.h"
#include "plist.h"
#include "z80mem.h"
#include "z80.h"
#include "userkybd.h"
#include "speech.h"
#include "teletext.h"
#include "csw.h"
#include "serialdevices.h"
#include "Arm.h"
#include "printing.h"
#include "discedit.h"

// #include "keytable_2"

void i86_main(void);
void PrintScreen(void);
void CopyToClipBoardAsPDF(int starty, int nlines);
void ProcessPrintCommand(int starty, int nlines);


#define VIDEO_CODEC kVideoCodecType
#define VIDEO_QUALITY codecMinQuality

// some LED based macros

#define LED_COLOUR_TYPE (LEDByte&4)>>2
#define LED_SHOW_KB		(LEDByte&1)
#define LED_SHOW_DISC	(LEDByte&2)>>1
#define LED_COL_BASE	64

FILE *CMDF2;
unsigned char CMA2;
CArm *arm = NULL;

unsigned char HideMenuEnabled;
bool MenuOn;

struct LEDType LEDs;
char DiscLedColour=0; // 0 for red, 1 for green.

bool m_PageFlipping=0;

bool DiscLoaded[2]={false,false}; // Set to TRUE when a disc image has been loaded.
char CDiscName[2][256]; // Filename of disc current in drive 0 and 1;
char CDiscType[2]; // Current disc types

static const char *WindowTitle = "BeebEm - BBC Model B / Master 128 Emulator";


// Row,Col - Physical mapping
int transTable1[256][2]={
	4,1,	5,1,	3,2,	4,3,   // 0  ASDF
	5,4,	5,3,	6,1,	4,2,   // 4  HGZX
	5,2,	6,3,	4,7,	6,4,   // 8  CV@B
	1,0,	2,1,	2,2,	3,3,   // 12 QWER
	4,4,	2,3,	3,0,	3,1,   // 16 YT12
	1,1,	1,2,	3,4,	1,3,   // 20 3465
	1,8,	2,6,	2,4,	1,7,   // 24 =97-
	1,5,	2,7,	5,8,	3,6,   // 28 80]O
	3,5,	3,8,	2,5,	3,7,   // 32 U[IP
	4,9,	5,6,	4,5,	4,8,   // 36 <CR>LJ'
	4,6,	5,7,	2,8,	6,6,   // 40 K;\,
	6,8,	5,5,	6,5,	6,7,   // 44 /NM.
	6,0,	6,2,	7,8,	5,9,   // 48 <TAB><SPC>`<BS>   
	0,0,	7,0,	0,0,	0,0,   // 52 <ESC>
	0,0,	0,0,	0,0,	0,0,   // 56
	0,0,	0,0,	0,0,	0,0,   // 60
	0,0,	0,0,	0,0,	0,0,   // 64C
	0,0,	0,0,	0,0,	0,0,   // 68
	0,0,	0,0,	0,0,	0,0,   // 72
	0,0,	0,0,	0,0,	0,0,   // 76
	0,0,	0,0,	0,0,	0,0,   // 80
	0,0,	0,0,	0,0,	0,0,   // 84
	0,0,	0,0,	0,0,	0,0,   // 88
	0,0,	0,0,	0,0,	0,0,   // 92
	7,4,	7,5,	1,6,	7,3,   // 96  F5F6F3F7
	7,6,	7,7,	0,0,	2,0,   // 100 F8F9.F11
	0,0,	-2,-2,	0,0,	0,0,   // 104 [Break - F13]
	0,0,	2,0,	0,0,	-2,-2,   // 108 .F10.[Break - F12]
	0,0,	0,0,	0,0,	0,0,   // 112
	0,0,	5,9,	1,4,	6,9,   // 116 .[Del]F4[End]
	7,2,	0,0,	7,1,	1,9,   // 120 F2 F1[left]
	7,9,	2,9,	3,9,	0,0,   // 124 [Right][Down][Up]
	0,0,	0,0,	0,0,	0,0,   // 128
	0,0,	0,0,	0,0,	0,0,   // 132
	0,0,	0,0,	0,0,	0,0,   // 136
	0,0,	0,0,	0,0,	0,0,   // 140
	0,0,	0,0,	0,0,	0,0,   // 144
	0,0,	0,0,	0,0,	0,0,   // 148
	0,0,	0,0,	0,0,	0,0,   // 152
	0,0,	0,0,	0,0,	0,0,   // 156
	0,0,	0,0,	0,0,	0,0,   // 160
	0,0,	0,0,	0,0,	0,0,   // 164
	0,0,	0,0,	0,0,	0,0,   // 168
	0,0,	0,0,	0,0,	0,0,   // 172
	0,0,	0,0,	0,0,	0,0,   // 176
	0,0,	0,0,	0,0,	0,0,   // 180
	0,0,	0,0,	0,0,	0,0,   // 184
	0,0,	0,0,	0,0,	0,0,   // 188 
	0,0,	0,0,	0,0,	0,0,   // 192 
	0,0,	0,0,	0,0,	0,0,   // 196
	0,0,	0,1,	4,0,	0,0,   // 200 .[Ctrl][Caps]
	0,0,	0,0,	0,0,	0,0,   // 204
	0,0,	0,0,	0,0,	0,0,   // 208
	0,0,	0,0,	0,0,	0,0,   // 212
	0,0,	0,0,	0,0,	0,0,   // 216 
	0,0,	0,0,	0,0,	0,0,   // 220 
	0,0,	0,0,	0,0,	0,0,   // 224
	0,0,	0,0,	0,0,	0,0,   // 228
	0,0,	0,0,	0,0,	0,0,   // 232
	0,0,	0,0,	0,0,	0,0, 
	0,0,	0,0,	0,0,	0,0, 
	0,0,	0,0,	0,0,	0,0, 
	0,0,	0,0,	0,0,	0,0, 
	0,0,	0,0,	0,0,	0,0 
};

/* Currently selected translation table */
int (*transTable)[2] = transTable1;
long WindowPos[4];

void BeebWin::SetupClipboard(void)
{
	m_OSRDCH = BeebReadMem(0x210) + (BeebReadMem(0x211) << 8);

	BeebWriteMem(0x100, 0x38);	// SEC
	BeebWriteMem(0x101, 0xad);	// LDA &FC51
	BeebWriteMem(0x102, 0x51);
	BeebWriteMem(0x103, 0xfc);
	BeebWriteMem(0x104, 0xd0);	// BNE P% + 6
	BeebWriteMem(0x105, 0x04);
	BeebWriteMem(0x106, 0xad);	// LDA &FC50
	BeebWriteMem(0x107, 0x50);
	BeebWriteMem(0x108, 0xfc);
	BeebWriteMem(0x109, 0x18);	// CLC
	BeebWriteMem(0x10A, 0x60);	// RTS

	BeebWriteMem(0x210, 0x00);
	BeebWriteMem(0x211, 0x01);

	KeyDown(36);		// Just to kick off keyboard input, 36 = return key

}

void BeebWin::ResetClipboard(void)
{
	BeebWriteMem(0x210, m_OSRDCH & 255);
	BeebWriteMem(0x211, m_OSRDCH >> 8);
}

int BeebWin::PasteKey(int addr)
{
int data = 0x00;
	
	switch (addr)
	{
		case 0 :
			KeyUp(36);
			if (m_clipboardlen > 0)
			{
				data = m_clipboard[m_clipboardptr++];
				m_clipboardlen--;
				if (m_clipboardlen <= 0)
				{
					ResetClipboard();
				}
			}
			break;
		case 1 :
			data = (m_clipboardlen == 0) ? 255 : 0;
			break;
	}
	return data;
}

void BeebWin::CopyKey(int Value)
{
//	fprintf(stderr, "Print %d (%c) to clipboard\n", Value, Value);

	PasteboardRef outPasteboard;
	OSStatus err;
    CFMutableDataRef    returnData = NULL;
	UInt8*              returnBytes;
	int					i;
	
	if (m_printerbufferlen >= 1024 * 1024)
		return;

	m_printerbuffer[m_printerbufferlen++] = Value;
	
	err = PasteboardCreate(kPasteboardClipboard, &outPasteboard);
	
	err = PasteboardClear( outPasteboard );
	
	if (m_printerbufferlen > 5)		// Don't paste initial L.<cr> command or final <cr> > prompt
	{
		
		returnData = CFDataCreateMutable( kCFAllocatorDefault, m_printerbufferlen - 4);
		
		returnBytes = CFDataGetMutableBytePtr( returnData );
		
		CFDataSetLength( returnData, m_printerbufferlen - 4);
		
		for( i = 0; i < m_printerbufferlen - 4; i++ )
			returnBytes[i] = m_printerbuffer[i + 3];
		
		PasteboardPutItemFlavor( outPasteboard, (PasteboardItemID)1,
							   CFSTR("com.apple.traditional-mac-plain-text"), returnData, 0 );

		CFRelease(returnData);
	}

	CFRelease(outPasteboard);

}


int BeebWin::KeyDown(int vkey)
{
int row, col;
int i;
int mask = 0x01;
bool bit = false;

	if (mEthernetPortWindow) return 0;
	
	if (mBreakOutWindow)
	{
		for (i = 0; i < 8; ++i)
		{
			if (BitKeys[i] == vkey)
			{
				if ((UserVIAState.ddrb & mask) == 0x00)
				{
					UserVIAState.irb &= ~mask;
					ShowInputs( (UserVIAState.orb & UserVIAState.ddrb) | (UserVIAState.irb & (~UserVIAState.ddrb)) );
					bit = true;
				}
			}
			mask <<= 1;
		}
	}
	
	if (bit == true) return 0;
	
	// Reset shift state if it was set by Run Disc
	if (m_ShiftBooted)
	{
		m_ShiftBooted = false;
		BeebKeyUp(0, 0);
	}

	TranslateKey(vkey, false, row, col);

	return 0;
}

int BeebWin::KeyUp(int vkey)
{
int row, col;
int i;
int mask = 0x01;
bool bit = false;

	if (mEthernetPortWindow) return 0;

	if (mBreakOutWindow)
	{
		for (i = 0; i < 8; ++i)
		{
			if (BitKeys[i] == vkey)
			{
				if ((UserVIAState.ddrb & mask) == 0x00)
				{
					UserVIAState.irb |= mask;
					ShowInputs( (UserVIAState.orb & UserVIAState.ddrb) | (UserVIAState.irb & (~UserVIAState.ddrb)) );
					bit = true;
				}
			}
			mask <<= 1;
		}
	}

	if (bit == true) return 0;
	
	if(TranslateKey(vkey, true, row, col) < 0)
	{
		if(row==-2)
		{ // Must do a reset!
			Init6502core();
			if ( (EnableTube) && (TubeEnabled) ) Init65C02core();
			if (Tube186Enabled) i86_main();
			Enable_Z80 = 0;
			if (TorchTube || AcornZ80)
			{
				R1Status = 0;
				ResetTube();
				init_z80();
				Enable_Z80 = 1;
			}
			Enable_Arm = 0;
			if (ArmTube)
			{
				R1Status = 0;
				ResetTube();
				if (arm) delete arm;
				arm = new CArm;
				Enable_Arm = 1;
			}
			Disc8271_reset();
			Reset1770();
			if (EconetEnabled) EconetReset();
			if (HardDriveEnabled) SCSIReset();
			if (HardDriveEnabled) SASIReset();
			if (TeleTextAdapterEnabled) TeleTextInit();

			//SoundChipReset();
		}
	}

	return 0;
}

/****************************************************************************/
int BeebWin::TranslateKey(int vkey, int keyUp, int &row, int &col)
{
	// Key track of shift state
	if (vkey == 200)
	{
		if (keyUp)
			m_ShiftPressed = false;
		else
			m_ShiftPressed = true;
	}

	if (keyUp)
	{
		// Key released, lookup beeb row + col that this vkey 
		// mapped to when it was pressed.  Need to release
		// both shifted and non-shifted presses.
		row = m_vkeyPressed[vkey][0][0];
		col = m_vkeyPressed[vkey][1][0];
		m_vkeyPressed[vkey][0][0] = -1;
		m_vkeyPressed[vkey][1][0] = -1;
		if (row >= 0)
			BeebKeyUp(row, col);

		row = m_vkeyPressed[vkey][0][1];
		col = m_vkeyPressed[vkey][1][1];
		m_vkeyPressed[vkey][0][1] = -1;
		m_vkeyPressed[vkey][1][1] = -1;
		if (row >= 0)
			BeebKeyUp(row, col);
	}
	else // New key press - convert to beeb row + col
	{
		int needShift = m_ShiftPressed;

		row = transTable[vkey][0];
		col = transTable[vkey][1];

		if (m_KeyMapAS)
		{
			// Map A & S to CAPS & CTRL - good for some games
			if (vkey == 0)				// A
			{
				row = 4;
				col = 0;
			} else if (vkey == 1)		// S
			{
				row = 0;
				col = 1;
			}
		}
		
		if (row >= 0)
		{
			// Make sure shift state is correct
			if (needShift)
				BeebKeyDown(0, 0);
			else
				BeebKeyUp(0, 0);

			BeebKeyDown(row, col);

			// Record beeb row + col for key release
			m_vkeyPressed[vkey][0][m_ShiftPressed ? 1 : 0] = row;
			m_vkeyPressed[vkey][1][m_ShiftPressed ? 1 : 0] = col;
		}
		else
		{
			// Special key!  Record so key up returns correct codes
			m_vkeyPressed[vkey][0][1] = row;
			m_vkeyPressed[vkey][1][1] = col;
		}
	}

	return(row);
}

BeebWin::BeebWin() 
{
  IBNibRef 		nibRef;
  OSStatus		err;

  // Create a Nib reference passing the name of the nib file (without the .nib extension)
  // CreateNibReference only searches into the application bundle.
  err = CreateNibReference(CFSTR("main"), &nibRef);
  if (err)
  {
	fprintf(stderr, "CreateNibReference : %ld\n", err);
  }
    
  // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar
  // object. This name is set in InterfaceBuilder when the nib is created.
  err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
  if (err)
  {
	fprintf(stderr, "SetMenuBarFromNib : %ld\n", err);
  }

  // Then create a window. "MainWindow" is the name of the window object. This name is set in 
  // InterfaceBuilder when the nib is created.
  err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), &mWindow);
  if (err)
  {
	fprintf(stderr, "CreateWindowFromNib : %ld\n", err);
  }

  // We don't need the nib reference anymore.
  DisposeNibReference(nibRef);
	  
  SetPortWindowPort(mWindow);

  DataSize=640*512;
  m_screen = (char *) malloc(800 * 512);
  m_screen_blur = (char *) malloc(800 * 512);

  fprintf(stderr, "Base Address = %08x\n", (unsigned int) m_screen);

  mBitMap.baseAddr = m_screen;
  mBitMap.rowBytes = 800 | 0x8000;
  mBitMap.bounds.left = 0;
  mBitMap.bounds.top = 0;
  mBitMap.bounds.right = 640;
  mBitMap.bounds.bottom = 512;
  mBitMap.pmVersion = 0;
  mBitMap.packType = 0;
  mBitMap.packSize = 0;
  mBitMap.hRes = 0x00480000;
  mBitMap.vRes = 0x00480000;
  mBitMap.pixelType = 0;
  mBitMap.pixelSize = 8;
  mBitMap.cmpCount = 1;
  mBitMap.cmpSize = 8;

  mCT = (CTabPtr) malloc(sizeof(ColorTable) + (LED_COL_BASE + 4) * sizeof(CSpecArray));
  mCT->ctSeed = 00;
  mCT->ctFlags = 0x4000;
  mCT->ctSize = LED_COL_BASE + 4 - 1;

  UpdatePalette(RGB);
  
  ShowWindow(mWindow);

CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:
 
;

}; /* BeebWin */

BeebWin::~BeebWin() 
{
  if (SoundEnabled)
	SoundReset();
	
  free(m_screen);
  free(m_screen_blur);

}; /* Destructor */

extern int done;

void BeebWin::doHorizLine(unsigned long Col, int y, int sx, int width) 
{
int d, e;
char *p;

	if (TeletextEnabled) 
	{
		e = sx + 80;
	}
	else
	{
		e = sx;
	}

	if (e < 0)
	{
		width = width - e;
		e = 0;
	}

	if (e + width > 799) width = 800 - e;
	if (width <= 0) return;

	d = (y*800)+ e + ScreenAdjust;
	if ((d+width)>(512*800)) return;
	if (d<0) return;
	p = m_screen + d;

	memset(m_screen + d, Col, width);
};

void BeebWin::doInvHorizLine(unsigned long Col, int y, int sx, int width) 
{
int d;
int e;
char *vaddr;

	if (TeletextEnabled)
	{
		e = sx + 80;
	}
	else
	{
		e = sx;
	}

	if (e < 0)
	{
		width = width - e;
		e = 0;
	}

	if (e + width > 799) width = 800 - e;
	if (width <= 0) return;

	d = (y*800) + e + ScreenAdjust;

	if ((d+width)>(512*800)) return;
	if (d<0) return;
	vaddr=m_screen+d;

	for (int n=0;n<width;n++)
	{
		vaddr[n] ^= Col;
	}
	
};

void BeebWin::doUHorizLine(unsigned long Col, int y, int sx, int width) 
{
	if (y>500) return;
    memset(m_screen + y*800 + sx, Col, width);
};

EightUChars *BeebWin::GetLinePtr(int y) 
{
	int d = (y*800) + ScreenAdjust;
	if (d > (MAX_VIDEO_SCAN_LINES*800))
		return((EightUChars *)(m_screen+(MAX_VIDEO_SCAN_LINES*800)));
	return((EightUChars *)(m_screen + d));
}

SixteenUChars *BeebWin::GetLinePtr16(int y) 
{
	int d = (y*800) + ScreenAdjust;
	if (d > (MAX_VIDEO_SCAN_LINES*800))
		return((SixteenUChars *)(m_screen+(MAX_VIDEO_SCAN_LINES*800)));
	return((SixteenUChars *)(m_screen + d));
}

static OSStatus blittask1(void *parameter);

void BeebWin::updateLines(int starty, int nlines) 
{
CGrafPtr mWin;

// OSStatus err;
// MPTaskID taskID;
// unsigned long data;

//	data = (starty << 16) | nlines;
//	err = MPCreateTask(blittask1, (void *) data, 0, nil, nil, nil, 0, &taskID);

	if (quitNow) return;		// Don't repaint if shutting down program

	if (m_FreezeWhenInactive)
		if (IsWindowCollapsed(mWindow)) return;	// Don't repaint if minimised
	
	bufferblit(starty, nlines); 
	return;

	++m_ScreenRefreshCount;

//	fprintf(stderr, "Refresh screen = %d\n", m_ScreenRefreshCount);
	
    Rect destR;
    Rect srcR;

	mWin = GetWindowPort(mWindow);
	SetPortWindowPort(mWindow);
	GetPortBounds(mWin, &destR);

	srcR.left = 0;
	srcR.right = 640;

	if (TeletextEnabled)
	{
		srcR.top = 0;
		srcR.bottom = 512;
	}
	else
	{
		srcR.top = starty;
		srcR.bottom = starty + nlines;
	}
	
	CopyBits ( (BitMap *) &mBitMap, GetPortBitMapForCopyBits(mWin), &srcR, &destR, srcCopy, nil);

}; /* BeebWin::updateLines */

CGContextRef MyCreateBitmapContext (int pixelsWide, int pixelsHigh, int bpp)
{
	CGContextRef context = NULL;
	CGColorSpaceRef colorSpace;
	void *bitmapData;
	int bitmapByteCount;
	int bitmapBytesPerRow;
	
	//	fprintf(stderr, "Creating a bitmap %d by %d\n", pixelsWide, pixelsHigh);
	
	switch (bpp)
	{
		case 32 :
			bitmapBytesPerRow = pixelsWide * 4;
			break;
		case 16 :
			bitmapBytesPerRow = pixelsWide * 2;
			break;
	}

	bitmapByteCount = (bitmapBytesPerRow * pixelsHigh);
	
	//	colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	
	colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	
	bitmapData = malloc(bitmapByteCount);
	if (bitmapData == NULL)
	{
		fprintf(stderr, "Memory not allocated !\n");
		return NULL;
	}
	
	switch (bpp)
	{
		case 32 :
			context = CGBitmapContextCreate(bitmapData, pixelsWide, pixelsHigh, 8, bitmapBytesPerRow, colorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
			break;
		case 16 :
			context = CGBitmapContextCreate(bitmapData, pixelsWide, pixelsHigh, 5, bitmapBytesPerRow, colorSpace, kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder16Host);
			break;
	}

	if (context == NULL)
	{
		free(bitmapData);
		fprintf(stderr, "Context not created!\n");
		return NULL;
	}
	
	CGColorSpaceRelease(colorSpace);
	
	return context;
}

void BeebWin::bufferblit1(int starty, int nlines) 
{
	CGrafPtr mWin;
	register int i, j;
	char *p;
	
	++m_ScreenRefreshCount;
	
	//	fprintf(stderr, "Refresh screen = %d\n", m_ScreenRefreshCount);
	
    Rect destR;
    Rect srcR;
	
	mWin = GetWindowPort(mWindow);
	SetPortWindowPort(mWindow);
	GetPortBounds(mWin, &destR);
	
	srcR.left = 0;
	srcR.right = 640;
	
	if (TeletextEnabled)
	{
		srcR.top = 0;
		srcR.bottom = 512;
	}
	else
	{
		srcR.top = starty;
		srcR.bottom = starty + nlines;
	}
	
	long *pPtr32;
	short *pPtr16;
	
	CGContextRef myBitmapContext;
	
	PixMapHandle	pmh;
	int				bpp;
	
	LockPortBits(mWin);
	pmh = GetPortPixMap(mWin);
	LockPixels(pmh);
	bpp = GetPixDepth(pmh);

	UnlockPixels(pmh);
	UnlockPortBits(mWin);

	myBitmapContext = MyCreateBitmapContext(srcR.right - srcR.left, srcR.bottom - srcR.top, bpp);
	
	pPtr32 = (long *) CGBitmapContextGetData (myBitmapContext);
	pPtr16 = (short *) CGBitmapContextGetData (myBitmapContext);
	
	//	fprintf(stderr, "Bitmap data ptr = %08x\n", (unsigned char *) pPtr);
	
	p = m_screen;
	
	//	fprintf(stderr, "top = %d, left = %d, bottom = %d, right = %d, bpr = %d, ppr = %d, scalex = %f, scaley = %f\n", 
	//		rect.top, rect.left, rect.bottom, rect.right, bpr, ppr, scalex, scaley);
	
	for (j = srcR.top; j < srcR.bottom; ++j)
	{
		p = m_screen + j * 800 + srcR.left;
		
		for (i = srcR.left; i < srcR.right; ++i)
		{
			switch (bpp)
			{
				case 32 :
					*pPtr32++ = m_RGB32[*p++];
					break;
				case 16 :
					*pPtr16++ = m_RGB16[*p++];
					break;
			}
		}
	}
	
	CGContextRef myContext;
	CGImageRef myImage;
	
	QDBeginCGContext(mWin, &myContext);
	
	myImage = CGBitmapContextCreateImage(myBitmapContext);
	
	CGContextDrawImage(myContext, CGRectMake(destR.left, destR.top, destR.right, destR.bottom), myImage);
	
	CGContextFlush(myContext);
	
	CGContextRelease(myBitmapContext);
	CGImageRelease(myImage);
	
	QDEndCGContext(mWin, &myContext);
	
}

void BeebWin::bufferblit2(int starty, int nlines) 
{
CGrafPtr mWin;
register int i, j;
char *p;

	++m_ScreenRefreshCount;

//	fprintf(stderr, "Refresh screen = %d\n", m_ScreenRefreshCount);
	
    Rect destR;
    Rect srcR;

	mWin = GetWindowPort(mWindow);
	SetPortWindowPort(mWindow);
	GetPortBounds(mWin, &destR);

	srcR.left = 0;
	srcR.right = 640;

	if (TeletextEnabled)
	{
		srcR.top = 0;
		srcR.bottom = 512;
	}
	else
	{
		srcR.top = starty;
		srcR.bottom = starty + nlines;
	}
	
long *pPtr;
CGColorSpaceRef colorSpace;
CGImageRef imageRef;
Ptr	dataPtr;
int w, h;
	
	w = srcR.right - srcR.left;
	h = srcR.bottom - srcR.top;
	
	colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	
	dataPtr = NewPtr( w * h * 4);
	
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, dataPtr, w * h * 4, NULL);
	if (provider == NULL)
	{
		fprintf(stderr, "Memory not allocated !\n");
		return;
	}
	
	imageRef = CGImageCreate(w, h, 8, 32, w * 4, colorSpace,
							 kCGImageAlphaPremultipliedLast,
							 provider,
							 NULL, 	// colourMap translation table
							 false,	// shouldInterpolate colours?
							 kCGRenderingIntentDefault);
	
	if (imageRef == NULL)
	{
		DisposePtr(dataPtr);
		fprintf(stderr, "Context not created!\n");
		return;
	}
	
	CGColorSpaceRelease(colorSpace);
	
	pPtr = (long *) dataPtr;

	p = m_screen;

	for (j = srcR.top; j < srcR.bottom; ++j)
	{
		p = m_screen + j * 800 + srcR.left;
		for (i = srcR.left; i < srcR.right; i++)
		{
			*pPtr++ = m_RGB32[*p++];
		}
	}

CGContextRef myContext;

	QDBeginCGContext(mWin, &myContext);

	CGContextDrawImage(myContext, CGRectMake(destR.left, destR.top, destR.right, destR.bottom), imageRef);

	CGContextFlush(myContext);

	CGImageRelease(imageRef);

	CGDataProviderRelease(provider);

	QDEndCGContext(mWin, &myContext);

	DisposePtr(dataPtr);
	
}

void BeebWin::bufferblit(int starty, int nlines) 
{
CGrafPtr mWin;
register int i, j;
char *p;
int width, height;
	

//	fprintf(stderr, "starty = %d, nlines = %d\n", starty, nlines);
	
	++m_ScreenRefreshCount;

	if (m_Motion_Blur > 0)
	{
		switch (m_Motion_Blur)
		{
			case 1 :			// 2 Frames
				j = 32;
				break;
			case 2 :			// 4 Frames
				j = 16;
				break;
			case 3 :			// 8 Frames
				j = 8;
				break;
		}
	
		for (i = 0; i < 800 * 512; ++i)
		{	
			if (m_screen[i] != 0)
			{
				m_screen_blur[i] = m_screen[i];
			}
			else if (m_screen_blur[i] != 0)
			{
				m_screen_blur[i] += j;
				if (m_screen_blur[i] > 63)
					m_screen_blur[i] = 0;
			}
		}
		memcpy(m_screen, m_screen_blur, 800 * 512);
	}
	
//	fprintf(stderr, "Refresh screen = %d\n", m_ScreenRefreshCount);
	
    Rect destR;
    Rect srcR;

	if (m_isFullScreen)
	{
		mWin = GetWindowPort(m_FullScreenWindow);
		SetPortWindowPort(m_FullScreenWindow);
	}
	else
	{
		mWin = GetWindowPort(mWindow);
		SetPortWindowPort(mWindow);
	}
	GetPortBounds(mWin, &destR);
	
	width = destR.right;
	height = destR.bottom;

	int xAdj = 0;
	int yAdj = 0;
	
	if (m_isFullScreen && m_maintainAspectRatio)
	{
		float m_XRatioCrop = 0.0f;
		float m_YRatioCrop = 0.0f;
		float m_XRatioAdj;
		float m_YRatioAdj;
		int w = width * 4;
		int h = height * 5;
		if (w > h)
		{ 
			m_XRatioAdj = (float) height / (float) width * (float) 5 / (float) 4;
			m_XRatioCrop = (1.0f - m_XRatioAdj) / 2.0f;
		}
		else if (w < h)
		{
			m_YRatioAdj = (float) width / (float) height * (float) 4 / (float) 5;
			m_YRatioCrop = (1.0f - m_YRatioAdj) / 2.0f;
		}
		xAdj = (int)(m_XRatioCrop * (float) (width));
		yAdj = (int)(m_YRatioCrop * (float) (height));
		
		width = width - 2 * xAdj;
		height = height - 2 * yAdj;
		
	}
	
	srcR.left = 0;
	srcR.right = ActualScreenWidth;

//	fprintf(stderr, "ActualScreenWidth = %d\n", ActualScreenWidth);

	if (TeletextEnabled)
	{
		srcR.top = 0;
		srcR.bottom = 512;
	}
	else
	{
		srcR.top = starty;
		srcR.bottom = starty + nlines;
	}
	
long *pPtr32;
long *pRPtr32;
short *pPtr16;
short *pRPtr16;

PixMapHandle	pmh;
Ptr             buffer;
int				bpr;
float			scalex;
float			scaley;
int				ppr;
Rect			rect;
int				bpp;

	LockPortBits(mWin);

	pmh = GetPortPixMap(mWin);

	LockPixels(pmh);

	bpr = GetPixRowBytes(pmh);
	buffer = GetPixBaseAddr(pmh);
	bpp = GetPixDepth(pmh);

//	fprintf(stderr, "bpp = %d\n", bpp);
	
	GetPixBounds(pmh, &rect);

	if (bpp == 32)
	{
		ppr = bpr / 4;
	}
	else if (bpp == 16)
	{
		ppr = bpr / 2;
	}
	else
	{
		ppr = bpr;
	}
	
//	fprintf(stderr, "dest : top = %d, left = %d, bottom = %d, right = %d\n", destR.top, destR.left, destR.bottom, destR.right);
//	fprintf(stderr, "rect : top = %d, left = %d, bottom = %d, right = %d\n", rect.top, rect.left, rect.bottom, rect.right);

	if (rect.top != 0)		// running full screen - don't paint !
	{
	
		p = m_screen;

		pRPtr32 = (long *) (buffer - rect.top * bpr - rect.left * 4 - yAdj * bpr);		// Skip past rows for window's menu bar, rect.top = -22 (on my system), plus any left margin
		pRPtr16 = (short *) (buffer - rect.top * bpr - rect.left * 2 - yAdj * bpr);		// Skip past rows for window's menu bar, rect.top = -22 (on my system)
	
		scalex = (float) ((srcR.right - srcR.left)) / (float) ((width));
		scaley = (float) ((srcR.bottom - srcR.top)) / (float) ((height));
	

// Pre-calculate the x scaling factor for speed

int sx[2000];

		for (i = 0; i < width; ++i)
		{
			sx[i] = (int) (i * scalex);
		}

		switch (bpp)
		{
			case 32 :
				for (j = 0; j < height; ++j)
				{
					p = m_screen + (srcR.top + (int) (j * scaley)) * 800 + srcR.left;
					pPtr32 = pRPtr32 + xAdj;
					for (i = 0; i < width; ++i)
						*pPtr32++ = m_RGB32[p[sx[i]]];
					
					pRPtr32 += ppr;
					
				}
				break;
			case 16 :
				for (j = 0; j < height; ++j)
				{
					p = m_screen + (srcR.top + (int) (j * scaley)) * 800 + srcR.left;
					pPtr16 = pRPtr16 + xAdj;
					
					for (i = 0; i < width; ++i)
						*pPtr16++ = m_RGB16[p[sx[i]]];
					
					pRPtr16 += ppr;
					
				}
				break;
		}
		
/*			
		
		for (j = destR.top; j < destR.bottom; ++j)
		{
			p = m_screen + (srcR.top + (int) (j * scaley)) * 800 + srcR.left;

			for (i = destR.left; i < destR.right; ++i)
			{
				switch (bpp)
				{
					case 32 :
						*pPtr32++ = m_RGB32[p[sx[i]]];
						break;
					case 16 :
						*pPtr16++ = m_RGB16[p[sx[i]]];
						break;
				}
			}

			pPtr32 += (ppr - destR.right);
			pPtr16 += (ppr - destR.right);

		}
*/
		
	}
	
OSErr err;

	if (m_PrintScreen)
	{

		if (TeletextEnabled)
		{
			DumpScreen(0);
		}
		else
		{
			DumpScreen(starty);
		}
		
		m_PrintScreen = false;
	}

	if (m_PrintToPDF)
	{
		
		if (TeletextEnabled)
		{
			ProcessPrintCommand(0, 512);
		}
		else
		{
			ProcessPrintCommand(starty, nlines);
		}
		
		m_PrintToPDF = false;
	}

	if (m_CopyToClip)
	{
		
		if (TeletextEnabled)
		{
			CopyToClipBoardAsPDF(0, 512);
		}
		else
		{
			CopyToClipBoardAsPDF(starty, nlines);
		}
		
		m_CopyToClip = false;
	}
	
	
	if (m_pMovie)
	{

		if (m_firstFrame)
		{
			m_firstFrame = false;
			err = SCCompressSequenceBegin(m_ci, pmh, &m_trackFrame, &m_imageDesc);
			if (err != noErr) fprintf(stderr, "SCCompressSequenceBegin error %d\n", err);
			m_frameDuration = 0;
			m_frameSkipCount = 0;
		}
	
		// 600 frames per second
		// current frame speed is m_FramesPerSecond
		
		if (m_frameSkipCount == 0)
		{
			short	flag = (m_keyFrameCount == m_keyFrame) ? 0 : mediaSampleNotSync;

//			flag = mediaSampleNotSync;
		
// Bug in H264 codec crashes system after an -8960 error
// so disable video capture after getting any error
			
			err = SCCompressSequenceFrame(m_ci, pmh, &m_trackFrame, &m_compressedData, &m_frameSize, &flag);
			if (err != noErr) 
			{
				fprintf(stderr, "SCCompressSequenceFrame error %d\n", err);
				EndCaptureVideo();
				goto bye;
			}

		}

		m_frameDuration += m_soundBufferLen;
		
		m_frameSkipCount++;
		if (m_frameSkipCount > m_frameSkip)
		{
			m_frameSkipCount = 0;

			m_keyFrameCount--;
			if (m_keyFrameCount <= 0)
				m_keyFrameCount = m_keyFrame;

			err = AddMediaSample(m_pMedia, m_compressedData, 0,  m_frameSize, m_frameDuration,
								 (SampleDescriptionHandle) m_imageDesc, 
								 1,  /* one sample */
								 mediaSampleNotSync,  /* self-contained samples */
								 nil);
			
			if (err != noErr) 
			{
				fprintf(stderr, "AddMediaSample Video error %d\n", err);
				EndCaptureVideo();
				goto bye;
			}
			
			m_frameDuration = 0;
		}
		
// Send sound to quicktime movie file

		err = AddMediaSample(m_sMedia, m_soundBuffer, 0, m_soundBufferLen, 
					 1,
					 (SampleDescriptionHandle) m_soundDesc, 
					 m_soundBufferLen,  /* samples per sec */
					 mediaSampleNotSync,
					 nil);

		if (err != noErr) 
		{
			fprintf(stderr, "AddMediaSample Sound error %d\n", err);
			EndCaptureVideo();
		}
		
		m_soundBufferLen = 0;

	}
	
bye: ;
	
	UnlockPixels(pmh);
	UnlockPortBits(mWin);
	
// Start a thread to flush buffer to screen ?

// OSStatus err;
// MPTaskID taskID;

//	err = MPCreateTask(blittask, nil, 0, nil, nil, nil, 0, &taskID);

RgnHandle reg;

	reg = NewRgn();
	
	SetRectRgn(reg, destR.left, destR.top, destR.right, destR.bottom);

	QDFlushPortBuffer(mWin, reg);

}

void BeebWin::DumpScreen(int offset)
{
FILE *f;
int i, j, x, y1, y2, s, c;
char *p;
char FileName[256];
	
	SystemSoundPlay(0);
		
	p = m_screen;
	x = 640;
	if (TeletextEnabled)
	{
		y1 = offset;
		y2 = offset + 510;
		s = 2;
	}
	else
	{
		y1 = offset;
		y2 = offset + 255;
		s = 1;
	}

	m_PicNum++;
	
	f = (FILE *) 1;
	
	while (f != NULL)
	{
		sprintf(FileName, "%spics/pic%d.bmp", RomPath, m_PicNum);

		f = fopen(FileName, "rb");
		if (f)
		{
			fclose(f);
			m_PicNum++;
		}
	}
	
	fprintf(stderr, "Writing screen dump %s\n", FileName);
	
	f = fopen(FileName, "wb");
	if (f == NULL) return;

	fputc('B', f);		// Bitmap Header
	fputc('M', f);

	fputc(0x76, f);		// Size of file, 0xa076 = 41078 bytes
	fputc(0xa0, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x00, f);		// Reserved
	fputc(0x00, f);
	fputc(0x00, f);		// Reserved
	fputc(0x00, f);

	fputc(0x76, f);		// Offset into file for beginning of bitmap data, 0x76 = 118
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x28, f);		// Size of bitmap header structure in bytes
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);
	
	fputc(0x40, f);		// Width of image in pixels, 0x0140 = 320
	fputc(0x01, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x00, f);		// Height of image in pixels, 0x0100 = 256
	fputc(0x01, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x01, f);		// Number of planes, 1
	fputc(0x00, f);

	fputc(0x04, f);		// Number of bits per pixel, 4
	fputc(0x00, f);

	fputc(0x00, f);		// No compression
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x00, f);		// Compressed file size
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x00, f);		// XPels per meter
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x00, f);		// YPels per meter
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x00, f);		// Colours used in bitmap
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);

	fputc(0x00, f);		// Number of important colours
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0x00, f);

// RGB Quad colour array 
	
	fputc(0x00, f);		// Black
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0xff, f);

	fputc(0x00, f);		// Red
	fputc(0x00, f);
	fputc(0xff, f);
	fputc(0xff, f);
	
	fputc(0x00, f);		// Green
	fputc(0xff, f);
	fputc(0x00, f);
	fputc(0xff, f);
	
	fputc(0x00, f);		// Yellow
	fputc(0xff, f);
	fputc(0xff, f);
	fputc(0xff, f);
	
	fputc(0xff, f);		// Blue
	fputc(0x00, f);
	fputc(0x00, f);
	fputc(0xff, f);

	fputc(0xff, f);		// Magenta
	fputc(0x00, f);
	fputc(0xff, f);
	fputc(0xff, f);
	
	fputc(0xff, f);		// Cyan
	fputc(0xff, f);
	fputc(0x00, f);
	fputc(0xff, f);
	
	fputc(0xff, f);		// White
	fputc(0xff, f);
	fputc(0xff, f);
	fputc(0xff, f);
	
	for (i = 0; i < 8; ++i)
	{
		fputc(0x00, f);		// Unused
		fputc(0x00, f);
		fputc(0x00, f);
		fputc(0x00, f);
	}
	
	for (j = y2; j >= y1; j -= s)
	{
		for (i = 0; i < x; i += 4)
		{
			c = ( ((m_screen[i + j * 800]) & 0x0f) << 4) | (m_screen[i + 2 + j * 800] & 0x0f);
			fputc(c, f);
		}
	}

	fclose(f);
}

static OSStatus blittask1(void *parameter)
{
unsigned long data;

	data = (unsigned long) parameter;
	
	mainWin->bufferblit((data >> 16), data & 0xffff);

	return noErr;
}

static OSStatus blittask(void *parameter)
{
RgnHandle reg;
CGrafPtr mWin;

    Rect destR;

	mWin = GetWindowPort(mainWin->mWindow);
	SetPortWindowPort(mainWin->mWindow);
	GetPortBounds(mWin, &destR);

	reg = NewRgn();
	SetRectRgn(reg, destR.left, destR.top, destR.right, destR.bottom);

	QDFlushPortBuffer(mWin, reg);
	return noErr;
}


/****************************************************************************/
void BeebWin::Initialise(char *home)
{   
	m_PicNum = 0;
	m_LastTickCount = 0;
	m_KeyMapAS = 0;
	m_KeyMapFunc = 0;
	m_ShiftPressed = 0;
	m_ShiftBooted = false;
	for (int k = 0; k < 256; ++k)
	{
		m_vkeyPressed[k][0][0] = -1;
		m_vkeyPressed[k][1][0] = -1;
		m_vkeyPressed[k][0][1] = -1;
		m_vkeyPressed[k][1][1] = -1;
	}

#ifdef DEBUG

	strcpy(RomPath, "/users/jon/myprojects/beebem/beebem4/");

	fprintf(stderr, "DEBUG\n");
	
#else

	fprintf(stderr, "RELEASE\n");

	strcpy(RomPath, home);
	char *p = strstr(RomPath, ".app");		// Find name of bundle
	if (p) *p = 0;
	p = strrchr(RomPath, '/');		// then come back to parent directory
	if (p) p[1] = 0; 

//	strcpy(RomPath, "/users/jonwelch/myprojects/beebem/beebem4/");
	
#endif

	strcpy(EconetCfgPath, RomPath);

	fprintf(stderr, "Home directory is '%s'\n", RomPath);

	LoadPreferences();

	// load the default user keymap if it is present
	char defaultUserKeymapPath [256];
	sprintf(defaultUserKeymapPath, "%sdefault.kmap", RomPath);
	if (LoadUserKeyboard(defaultUserKeymapPath))
	{
		fprintf(stderr, "Default User Defined Keyboard Mapping '%s' loaded\n", defaultUserKeymapPath);
		m_MenuIdKeyMapping = 1;
		TranslateKeyMapping();
	}
	
	TouchScreenOpen();
	
	IgnoreIllegalInstructions = 1;

	m_WriteProtectDisc[0] = !IsDiscWritable(0);
	m_WriteProtectDisc[1] = !IsDiscWritable(1);

	m_ScreenRefreshCount = 0;
	m_RelativeSpeed = 1;
	m_FramesPerSecond = 50;
	strcpy(m_szTitle, WindowTitle);

	for(int i = 0; i < LED_COL_BASE + 4; i++)
		cols[i] = i;

	m_frozen = false;

	MenuOn=true;

	InitMenu();

	if (PrinterEnabled)
		PrinterEnable(m_PrinterDevice);
	else
		PrinterDisable();
}

/****************************************************************************/
void BeebWin::SetRomMenu(void)
{
char Title[19];
int i;

MenuRef			menu = nil;
MenuItemIndex	j;
OSStatus		err;

	for (i = 0; i < 16; ++i)
	{
		sprintf(Title, "%1x ", i);
		ReadRomTitle(i, &Title[2], sizeof (Title) - 3);
		if (Title[2] == 0)
			strcpy(&Title[2], "Empty");

		err = GetIndMenuItemWithCommandID(nil, 'roma' + i, 1, &menu, &j);
		if (!err)
		{

			CFStringRef pTitle;
			pTitle = CFStringCreateWithCString (kCFAllocatorDefault, Title, kCFStringEncodingASCII);
			err = SetMenuItemTextWithCFString(menu, j, pTitle);

			if (err)
			{
				fprintf(stderr, "SetMenuTitle returned err code %d\n", (int) err);
			}
			CFRelease(pTitle);
		}
		else
		{
			fprintf(stderr, "Cannot find menu for Rom title %d\n", i);
		}
		
		SetMenuCommandIDCheck('roma' + i, RomWritable[i] ? true : false);
	}
}

/****************************************************************************/
void BeebWin::GetRomMenu(void)
{
int i;

	for (i = 0; i < 16; ++i)
	{
		SetMenuCommandIDCheck('roma' + i, RomWritable[i] ? true : false);
	}
}

void BeebWin::SetSoundMenu(void) {
}

/****************************************************************************/
void BeebWin::ResetBeebSystem(unsigned char NewModelType,unsigned char TubeStatus,unsigned char LoadRoms) 
{
	BeebReleaseAllKeys();
	SwitchOnCycles=0; // Reset delay
	SoundChipReset();
	SwitchOnSound();
	EnableTube=TubeStatus;
	MachineType=NewModelType;
	BeebMemInit(LoadRoms,m_ShiftBooted);
	Init6502core();
	if (EnableTube) Init65C02core();
	if (Tube186Enabled) i86_main();
	Enable_Z80 = 0;
	if (TorchTube || AcornZ80)
	{
		R1Status = 0;
		ResetTube();
		init_z80();
		Enable_Z80 = 1;
	}
	Enable_Arm = 0;
	if (ArmTube)
	{
		R1Status = 0;
		ResetTube();
		if (arm) delete arm;
		arm = new CArm;
		Enable_Arm = 1;
	}
	SysVIAReset();
	UserVIAReset();
	VideoInit();
	Disc8271_reset();
	if (EconetEnabled) EconetReset();
	Reset1770();
	AtoDInit();
	SetRomMenu();
	FreeDiscImage(0);
	// Keep the disc images loaded
	FreeDiscImage(1);
	Close1770Disc(0);
	Close1770Disc(1);
    if (HardDriveEnabled) SCSIReset();
    if (HardDriveEnabled) SASIReset();
    if (TeleTextAdapterEnabled) TeleTextInit();
	if (MachineType==3) InvertTR00=false;
	if ((MachineType!=3) && (NativeFDC)) {
		// 8271 disc
		if ((DiscLoaded[0]) && (CDiscType[0]==0)) LoadSimpleDiscImage(CDiscName[0],0,0,80);
		if ((DiscLoaded[0]) && (CDiscType[0]==1)) LoadSimpleDSDiscImage(CDiscName[0],0,80);
		if ((DiscLoaded[1]) && (CDiscType[1]==0)) LoadSimpleDiscImage(CDiscName[1],1,0,80);
		if ((DiscLoaded[1]) && (CDiscType[1]==1)) LoadSimpleDSDiscImage(CDiscName[1],1,80);
	}
	if (((MachineType!=3) && (!NativeFDC)) || (MachineType==3)) {
		// 1770 Disc
		if (DiscLoaded[0]) Load1770DiscImage(CDiscName[0],0,CDiscType[0]);
		if (DiscLoaded[1]) Load1770DiscImage(CDiscName[1],1,CDiscType[1]);
	}

	InitMenu();
}

void BeebWin::SetImageName(char *DiscName,char Drive,char DType) {
MenuRef			menu = nil;
MenuItemIndex	j;
OSStatus		err;
char			*fname;
char			Title[100];

	strcpy(CDiscName[Drive],DiscName);
	CDiscType[Drive]=DType;
	DiscLoaded[Drive]=true;

	fname = strrchr(CDiscName[Drive], '\\');
	if (fname == NULL)
		fname = strrchr(CDiscName[Drive], '/');
	if (fname == NULL)
		fname = CDiscName[Drive];
	else
		fname++;

	sprintf(Title, "Eject Disc %d: ", Drive);
	strncat(Title, fname, 99 - strlen(Title));
	Title[99] = 0;
	
	err = GetIndMenuItemWithCommandID(nil, 'ejd0' + Drive, 1, &menu, &j);
	if (!err)
	{
			
		CFStringRef pTitle;
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault, Title, kCFStringEncodingASCII);
		err = SetMenuItemTextWithCFString(menu, j, pTitle);
			
		if (err)
		{
			fprintf(stderr, "SetMenuTitle returned err code %ld\n", err);
		}
		CFRelease(pTitle);
	}
}

void BeebWin::EjectDiscImage(int Drive) {
MenuRef			menu = nil;
MenuItemIndex	j;
OSStatus		err;
char			Title[100];
	
	Eject8271DiscImage(Drive);
	Close1770Disc(Drive);
	
	strcpy(CDiscName[Drive], "");
	CDiscType[Drive] = 0;
	DiscLoaded[Drive] = FALSE;

	sprintf(Title, "Eject Disc %d", Drive);
	
	err = GetIndMenuItemWithCommandID(nil, 'ejd0' + Drive, 1, &menu, &j);
	if (!err)
	{
		
		CFStringRef pTitle;
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault, Title, kCFStringEncodingASCII);
		err = SetMenuItemTextWithCFString(menu, j, pTitle);
		
		if (err)
		{
			fprintf(stderr, "SetMenuTitle returned err code %d\n", (int) err);
		}
		CFRelease(pTitle);
	}
	
	if (Drive == 0)
	{
		SetMenuCommandIDCheck('wrp0', true);
	}
	else
	{
		SetMenuCommandIDCheck('wrp1', true);
	}

	SetDiscWriteProtects();
	
}

/****************************************************************************/
void BeebWin::SavePreferences()
{
CFURLRef fileURL;
char path[256];
CFStringRef pIni;
CFMutableDictionaryRef dict;
char temp[256];
int i;
CFStringRef pTitle;

	// Create a dictionary that will hold the data.
	dict = CFDictionaryCreateMutable( kCFAllocatorDefault,
            0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks );

// Put the various items into the dictionary.
// Because the values are retained as they are placed into the
//  dictionary, we can release any allocated objects here.

	AddDictNum(dict, CFSTR("MachineType"), MachineType);
	AddDictNum(dict, CFSTR("ShowFPS"), m_ShowSpeedAndFPS);
	AddDictNum(dict, CFSTR("TubeEnabled"), TubeEnabled);
	AddDictNum(dict, CFSTR("Tube186Enabled"), Tube186Enabled);
	AddDictNum(dict, CFSTR("TorchTube"), TorchTube);
	AddDictNum(dict, CFSTR("ArmTube"), ArmTube);
	AddDictNum(dict, CFSTR("AcornZ80"), AcornZ80);
	AddDictNum(dict, CFSTR("OpCodes"), OpCodes);
	AddDictNum(dict, CFSTR("BasicHardware"), BHardware);
	AddDictNum(dict, CFSTR("TeletextHalfMode"), THalfMode);
	AddDictNum(dict, CFSTR("SoundBlockSize"), SBSize);
	AddDictNum(dict, CFSTR("isFullScreen"), m_isFullScreen);
	AddDictNum(dict, CFSTR("MaintainAspectRatio"), m_maintainAspectRatio);
	AddDictNum(dict, CFSTR("WindowSize"), m_MenuIdWinSize);
	AddDictNum(dict, CFSTR("Monitor"), palette_type);
	AddDictNum(dict, CFSTR("MotionBlur"), m_Motion_Blur);
	AddDictNum(dict, CFSTR("LEDInformation"), (DiscLedColour << 2) | ((LEDs.ShowDisc?1:0)<<1) | (LEDs.ShowKB?1:0));
	AddDictNum(dict, CFSTR("Timing"), m_MenuIdTiming);
	AddDictNum(dict, CFSTR("EconetEnabled"), EconetEnabled);
	AddDictNum(dict, CFSTR("SpeechEnabled"), SpeechDefault);
	AddDictNum(dict, CFSTR("SoundEnabled"), SoundEnabled);
	AddDictNum(dict, CFSTR("SoundChipEnabled"), SoundChipEnabled);
	AddDictNum(dict, CFSTR("SampleRate"), m_MenuIdSampleRate);
	AddDictNum(dict, CFSTR("Volume"), m_MenuIdVolume);
	AddDictNum(dict, CFSTR("PartSamples"), PartSamples);
	AddDictNum(dict, CFSTR("Sticks"), m_MenuIdSticks);
	AddDictNum(dict, CFSTR("FreezeWhenInactive"), m_FreezeWhenInactive);
	AddDictNum(dict, CFSTR("HideCursor"), m_HideCursor);
	AddDictNum(dict, CFSTR("KeyMapping"), m_MenuIdKeyMapping);
	AddDictNum(dict, CFSTR("KeyMapAS"), m_KeyMapAS);
	AddDictNum(dict, CFSTR("KeyMapFunc"), m_KeyMapFunc);
	AddDictNum(dict, CFSTR("PrinterEnabled"), PrinterEnabled);
	AddDictNum(dict, CFSTR("PrinterPort"), m_MenuIdPrinterPort);
	AddDictString(dict, CFSTR("PrinterFile"), m_PrinterFileName);
	AddDictNum(dict, CFSTR("AMXMouseEnabled"), AMXMouseEnabled);
	AddDictNum(dict, CFSTR("AMXMouseLRForMiddle"), AMXLRForMiddle);
	AddDictNum(dict, CFSTR("AMXMouseSize"), m_MenuIdAMXSize);
	AddDictNum(dict, CFSTR("AMXMouseAdjust"), m_MenuIdAMXAdjust);
	AddDictNum(dict, CFSTR("UnlockTape"), UnlockTape);
	AddDictNum(dict, CFSTR("TapeClockSpeed"), TapeClockSpeed);
	AddDictNum(dict, CFSTR("TapeSoundEnabled"), TapeSoundEnabled);
	AddDictNum(dict, CFSTR("RelaySoundEnabled"), RelaySoundEnabled);
	AddDictNum(dict, CFSTR("DiscDriveSoundEnabled"), DiscDriveSoundEnabled);
	AddDictNum(dict, CFSTR("FrameSkip"), m_skip);
	AddDictNum(dict, CFSTR("CaptureResolution"), m_captureresolution);
	AddDictNum(dict, CFSTR("ExponentialVolume"), SoundExponentialVolume);
	AddDictNum(dict, CFSTR("TeleTextAdapterEnabled"), TeleTextAdapterEnabled);
	AddDictNum(dict, CFSTR("TeleTextData"), TeleTextData);
	AddDictNum(dict, CFSTR("TeleTextServer"), TeleTextServer);
	AddDictNum(dict, CFSTR("HardDriveEnabled"), HardDriveEnabled);
	AddDictNum(dict, CFSTR("SerialPortEnabled"), SerialPortEnabled);
	AddDictNum(dict, CFSTR("EthernetPortEnabled"), EthernetPortEnabled);
	AddDictNum(dict, CFSTR("TouchScreenEnabled"), TouchScreenEnabled);
	AddDictNum(dict, CFSTR("RTCEnabled"), RTC_Enabled);
	AddDictNum(dict, CFSTR("InvertBackground"), m_Invert);
	AddDictNum(dict, CFSTR("WriteProtectOnLoad"), m_WriteProtectOnLoad);
	AddDictNum(dict, CFSTR("NativeFDC"), NativeFDC);
	AddDictNum(dict, CFSTR("FDCType"), FDCType);
	
	for (i = 0; i < 256; ++i)
	{
		sprintf(temp, "Row%d", i);
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault,temp, kCFStringEncodingASCII);
		AddDictNum(dict, pTitle, UserKeymap[i][0]);
		CFRelease(pTitle);

		sprintf(temp, "Col%d", i);
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault,temp, kCFStringEncodingASCII);
		AddDictNum(dict, pTitle, UserKeymap[i][1]);
		CFRelease(pTitle);
	}
	
	for (i = 0; i < 8; ++i)
	{
		sprintf(temp, "BitKey%d", i);
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault,temp, kCFStringEncodingASCII);
		AddDictNum(dict, pTitle, BitKeys[i]);
		CFRelease(pTitle);
	}
	
	// Create a URL that specifies the file we will create to 
	// hold the XML data.

	sprintf(path, "%sbeebem.ini", RomPath);
	pIni = CFStringCreateWithCString (kCFAllocatorDefault, path, kCFStringEncodingASCII);

	fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault,    
               pIni,       // file path name
               kCFURLPOSIXPathStyle,    // interpret as POSIX path        
               false );                 // is it a directory?
			   
   // Write the property list to the file.
	WriteMyPropertyListToFile( dict, fileURL );
	CFRelease(dict);
	CFRelease(fileURL);

	CFRelease(pIni);
}

/****************************************************************************/
void BeebWin::LoadPreferences()
{
CFURLRef fileURL;
char path[256];
CFStringRef pIni;
CFMutableDictionaryRef dict;
CFStringRef pTitle;
int i;
char temp[256];

int LEDByte;

	// Create a URL that specifies the file we will create to 
	// hold the XML data.

	sprintf(path, "%sbeebem.ini", RomPath);
	pIni = CFStringCreateWithCString (kCFAllocatorDefault, path, kCFStringEncodingASCII);

	fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault,    
               pIni,       // file path name
               kCFURLPOSIXPathStyle,    // interpret as POSIX path        
               false );                 // is it a directory?

	if (fileURL == NULL)
	{
		fprintf(stderr, "Cannot find beebem.ini\n");
		exit(1);
	}

	dict = (CFMutableDictionaryRef) CreateMyPropertyListFromFile(fileURL); 

	if (dict == NULL)
	{
		fprintf(stderr, "Cannot create property file\n");
		exit(1);
	}

	NativeFDC = GetDictNum(dict, CFSTR("NativeFDC"), 1);
	FDCType = GetDictNum(dict, CFSTR("FDCType"), 0);
	TranslateFDC();

	m_WriteProtectOnLoad = GetDictNum(dict, CFSTR("WriteProtectOnLoad"), 1);
	MachineType = GetDictNum(dict, CFSTR("MachineType"), 3);
	m_isFullScreen = GetDictNum(dict, CFSTR("isFullScreen"), 0);
	m_maintainAspectRatio = GetDictNum(dict, CFSTR("MaintainAspectRatio"), 1);
	m_MenuIdWinSize = GetDictNum(dict, CFSTR("WindowSize"), 4);
	TranslateWindowSize(m_MenuIdWinSize);

	m_ShowSpeedAndFPS = GetDictNum(dict, CFSTR("ShowFPS"), 1);
	palette_type = (PaletteType) GetDictNum(dict, CFSTR("Monitor"), (PaletteType) RGB);

	m_Motion_Blur = GetDictNum(dict, CFSTR("MotionBlur"), 0);

	LEDByte = GetDictNum(dict, CFSTR("LEDInformation"), 7);

	DiscLedColour = LED_COLOUR_TYPE;
	LEDs.ShowDisc = LED_SHOW_DISC;
	LEDs.ShowKB = LED_SHOW_KB;

	m_MenuIdTiming = GetDictNum(dict, CFSTR("Timing"), IDM_REALTIME);
	TranslateTiming(m_MenuIdTiming);
	
	EconetEnabled = GetDictNum(dict, CFSTR("EconetEnabled"), 0);
	SpeechDefault = GetDictNum(dict, CFSTR("SpeechEnabled"), 0);
	SoundEnabled = GetDictNum(dict, CFSTR("SoundEnabled"), 1);
	SoundChipEnabled = GetDictNum(dict, CFSTR("SoundChipEnabled"), 1);
	m_MenuIdSampleRate = GetDictNum(dict, CFSTR("SampleRate"), IDM_22050KHZ);
	TranslateSampleRate();

	m_MenuIdVolume = GetDictNum(dict, CFSTR("Volume"), IDM_MEDIUMVOLUME);
	TranslateVolume();

	PartSamples = GetDictNum(dict, CFSTR("PartSamples"), 1);
	m_MenuIdSticks = GetDictNum(dict, CFSTR("Sticks"), 0);
	m_FreezeWhenInactive = GetDictNum(dict, CFSTR("FreezeWhenInactive"), 1);
	m_HideCursor = GetDictNum(dict, CFSTR("HideCursor"), 0);
	m_MenuIdKeyMapping = GetDictNum(dict, CFSTR("KeyMapping"), 0);
	m_KeyMapAS = GetDictNum(dict, CFSTR("KeyMapAS"), 0);
	m_KeyMapFunc = GetDictNum(dict, CFSTR("KeyMapFunc"), 0);
	TranslateKeyMapping();

	for (i = 0; i < 256; ++i)
	{
		sprintf(temp, "Row%d", i);
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault, temp, kCFStringEncodingASCII);
		UserKeymap[i][0] = GetDictNum(dict, pTitle, transTable1[i][0]);
		CFRelease(pTitle);
		
		sprintf(temp, "Col%d", i);
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault, temp, kCFStringEncodingASCII);
		UserKeymap[i][1] = GetDictNum(dict, pTitle, transTable1[i][1]);
		CFRelease(pTitle);
	}
	
	for (i = 0; i < 8; ++i)
	{
		sprintf(temp, "BitKey%d", i);
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault, temp, kCFStringEncodingASCII);
		BitKeys[i] = GetDictNum(dict, pTitle, BitKeys[i]);
		CFRelease(pTitle);
	}
	
	SoundExponentialVolume = GetDictNum(dict, CFSTR("ExponentialVolume"), 1);
	TeleTextAdapterEnabled = GetDictNum(dict, CFSTR("TeleTextAdapterEnabled"), 0);
	TeleTextData = GetDictNum(dict, CFSTR("TeleTextData"), 0);
	TeleTextServer = GetDictNum(dict, CFSTR("TeleTextServer"), 0);
	HardDriveEnabled = GetDictNum(dict, CFSTR("HardDriveEnabled"), 0);
	TubeEnabled = GetDictNum(dict, CFSTR("TubeEnabled"), 0);
	Tube186Enabled = GetDictNum(dict, CFSTR("Tube186Enabled"), 0);
	TorchTube = GetDictNum(dict, CFSTR("TorchTube"), 0);
	ArmTube = GetDictNum(dict, CFSTR("ArmTube"), 0);
	AcornZ80 = GetDictNum(dict, CFSTR("AcornZ80"), 0);
	OpCodes = GetDictNum(dict, CFSTR("OpCodes"), 2);
	BHardware = GetDictNum(dict, CFSTR("BasicHardware"), 0);
	THalfMode = GetDictNum(dict, CFSTR("TeletextHalfMode"), 0);
	SBSize = GetDictNum(dict, CFSTR("SoundBlockSize"), 0);
	m_Invert = GetDictNum(dict, CFSTR("InvertBackground"), 0);

	UnlockTape = GetDictNum(dict, CFSTR("UnlockTape"), 0);
	TapeClockSpeed = GetDictNum(dict, CFSTR("TapeClockSpeed"), 5600);
	TapeSoundEnabled = GetDictNum(dict, CFSTR("TapeSoundEnabled"), 1);
	RelaySoundEnabled = GetDictNum(dict, CFSTR("RelaySoundEnabled"), 1);
	DiscDriveSoundEnabled = GetDictNum(dict, CFSTR("DiscDriveSoundEnabled"), 1);

	PrinterEnabled = GetDictNum(dict, CFSTR("PrinterEnabled"), 0);
	m_MenuIdPrinterPort = GetDictNum(dict, CFSTR("PrinterPort"), IDM_PRINTER_FILE);
	GetDictString(dict, CFSTR("PrinterFile"), m_PrinterFileName, (char *) "");
	TranslatePrinterPort();

	AMXMouseEnabled = GetDictNum(dict, CFSTR("AMXMouseEnabled"), 0);
	AMXLRForMiddle = GetDictNum(dict, CFSTR("AMXMouseLRForMiddle"), 1);
	m_MenuIdAMXSize = GetDictNum(dict, CFSTR("AMXMouseSize"), 2);
	m_MenuIdAMXAdjust = GetDictNum(dict, CFSTR("AMXMouseAdjust"), 2);
	TranslateAMX();

	m_skip = GetDictNum(dict, CFSTR("FrameSkip"), 0);
	m_captureresolution = GetDictNum(dict, CFSTR("CaptureResolution"), 2);
	TranslateCapture();
	
	SerialPortEnabled = GetDictNum(dict, CFSTR("SerialPortEnabled"), 0);
	EthernetPortEnabled = GetDictNum(dict, CFSTR("EthernetPortEnabled"), 0);
	TouchScreenEnabled = GetDictNum(dict, CFSTR("TouchScreenEnabled"), 0);
	RTC_Enabled = GetDictNum(dict, CFSTR("RTCEnabled"), 0);

	SavePreferences();
}


/****************************************************************************/
void BeebWin::TranslateTiming(int id)
{
	m_FPSTarget = 0;
	m_RealTimeTarget = 1.0;
	m_MenuIdTiming = id;
	
	SetMenuCommandIDCheck('sRT ', false);
	SetMenuCommandIDCheck('s50 ', false);
	SetMenuCommandIDCheck('s25 ', false);
	SetMenuCommandIDCheck('s10 ', false);
	SetMenuCommandIDCheck('s5  ', false);
	SetMenuCommandIDCheck('s1  ', false);
	SetMenuCommandIDCheck('f100', false);
	SetMenuCommandIDCheck('f50 ', false);
	SetMenuCommandIDCheck('f10 ', false);
	SetMenuCommandIDCheck('f5  ', false);
	SetMenuCommandIDCheck('f2  ', false);
	SetMenuCommandIDCheck('f1.5', false);
	SetMenuCommandIDCheck('f1.2', false);
	SetMenuCommandIDCheck('f1.1', false);
	SetMenuCommandIDCheck('f0.9', false);
	SetMenuCommandIDCheck('f0.7', false);
	SetMenuCommandIDCheck('f0.5', false);
	SetMenuCommandIDCheck('f0.2', false);
	SetMenuCommandIDCheck('f0.1', false);

	if (m_MenuIdTiming == IDM_3QSPEED)
		m_MenuIdTiming = IDM_FIXEDSPEED0_75;
	if (m_MenuIdTiming == IDM_HALFSPEED)
		m_MenuIdTiming = IDM_FIXEDSPEED0_5;

	switch (m_MenuIdTiming)
	{
	default:
	case IDM_REALTIME:
		SetMenuCommandIDCheck('sRT ', true);
		m_RealTimeTarget = 1.0;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED100:
		SetMenuCommandIDCheck('f100', true);
		m_RealTimeTarget = 100.0;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED50:
		SetMenuCommandIDCheck('f50 ', true);
		m_RealTimeTarget = 50.0;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED10:
		SetMenuCommandIDCheck('f10 ', true);
		m_RealTimeTarget = 10.0;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED5:
		SetMenuCommandIDCheck('f5  ', true);
		m_RealTimeTarget = 5.0;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED2:
		SetMenuCommandIDCheck('f2  ', true);
		m_RealTimeTarget = 2.0;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED1_5:
		SetMenuCommandIDCheck('f1.5', true);
		m_RealTimeTarget = 1.5;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED1_25:
		SetMenuCommandIDCheck('f1.2', true);
		m_RealTimeTarget = 1.25;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED1_1:
		SetMenuCommandIDCheck('f1.1', true);
		m_RealTimeTarget = 1.1;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED0_9:
		SetMenuCommandIDCheck('f0.9', true);
		m_RealTimeTarget = 0.9;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED0_5:
		SetMenuCommandIDCheck('f0.5', true);
		m_RealTimeTarget = 0.5;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED0_75:
		SetMenuCommandIDCheck('f0.7', true);
		m_RealTimeTarget = 0.75;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED0_25:
		SetMenuCommandIDCheck('f0.2', true);
		m_RealTimeTarget = 0.25;
		m_FPSTarget = 0;
		break;

	case IDM_FIXEDSPEED0_1:
		SetMenuCommandIDCheck('f0.1', true);
		m_RealTimeTarget = 0.1;
		m_FPSTarget = 0;
		break;

	case IDM_50FPS:
		SetMenuCommandIDCheck('s50 ', true);
		m_FPSTarget = 50;
		m_RealTimeTarget = 0;
		break;

	case IDM_25FPS:
		SetMenuCommandIDCheck('s25 ', true);
		m_FPSTarget = 25;
		m_RealTimeTarget = 0;
		break;

	case IDM_10FPS:
		SetMenuCommandIDCheck('s10 ', true);
		m_FPSTarget = 10;
		m_RealTimeTarget = 0;
		break;

	case IDM_5FPS:
		SetMenuCommandIDCheck('s5  ', true);
		m_FPSTarget = 5;
		m_RealTimeTarget = 0;
		break;

	case IDM_1FPS:
		SetMenuCommandIDCheck('s1  ', true);
		m_FPSTarget = 1;
		m_RealTimeTarget = 0;
		break;
	}

	ResetTiming();
}


void BeebWin::AdjustSpeed(bool up)
{
}

/****************************************************************************/
void BeebWin::TranslateKeyMapping(void)
{
	switch (m_MenuIdKeyMapping)
	{
	default :
	case 0 :
		transTable = transTable1;
		break;
	case 1 :
		transTable = UserKeymap;
		break;
	}
}

/****************************************************************************/
void BeebWin::ResetTiming(void)
{
	m_LastTickCount = GetTickCount();
	m_LastStatsTickCount = m_LastTickCount;
	m_LastTotalCycles = TotalCycles;
	m_LastStatsTotalCycles = TotalCycles;
	m_TickBase = m_LastTickCount;
	m_CycleBase = TotalCycles;
	m_MinFrameCount = 0;
	m_LastFPSCount = m_LastTickCount;
	m_ScreenRefreshCount = 0;
}

/****************************************************************************/
void BeebWin::TranslateVolume(void)
{

	SetMenuCommandIDCheck('volf', false);
	SetMenuCommandIDCheck('volh', false);
	SetMenuCommandIDCheck('volm', false);
	SetMenuCommandIDCheck('voll', false);

	switch (m_MenuIdVolume)
	{
	case IDM_FULLVOLUME:
		SetMenuCommandIDCheck('volf', true);
		SoundVolume = 1;
		break;
	case IDM_HIGHVOLUME:
		SetMenuCommandIDCheck('volh', true);
		SoundVolume = 2;
		break;
	case IDM_MEDIUMVOLUME:
		SetMenuCommandIDCheck('volm', true);
		SoundVolume = 3;
		break;
	case IDM_LOWVOLUME:
		SetMenuCommandIDCheck('voll', true);
		SoundVolume = 4;
		break;
	}
}

/****************************************************************************/
void BeebWin::TranslateFDC(void)
{
	
	SetMenuCommandIDCheck('mbcn', NativeFDC);
	SetMenuCommandIDCheck('mbca', false);
	SetMenuCommandIDCheck('mbco', false);
	SetMenuCommandIDCheck('mbcw', false);

	if (NativeFDC == false)
	{
		switch(FDCType)
		{
			case 0 :
				SetMenuCommandIDCheck('mbca', true);
				EFDCAddr = 0xfe84;
				EDCAddr = 0xfe80;
				InvertTR00 = FALSE;
				break;
			case 1 :
				SetMenuCommandIDCheck('mbco', true);
				EFDCAddr = 0xfe80;
				EDCAddr = 0xfe84;
				InvertTR00 = FALSE;
				break;
			case 2 :
				SetMenuCommandIDCheck('mbcw', true);
				EFDCAddr = 0xfe84;
				EDCAddr = 0xfe80;
				InvertTR00 = TRUE;
				break;
		}
	}
		
}

void BeebWin::SetDriveControl(unsigned char value)
{

	unsigned char temp;
	
//	WriteLog("SetDriveControl - Type - %d, Value = %02x\n", FDCType, value);
	
	switch (FDCType)
	{
		case 0 :		// Acorn
			temp = value & 3;
			temp |= (value & 12) << 2;
			break;
		case 1 :		// Opus
			temp = (value & 2) << 3;
			temp |= ((value ^255) & 64) >> 1;
			if (value & 1) temp |= 2; else temp |= 1;
			break;
		case 2 :		// Watford
			if (value & 4) temp = 2; else temp = 1;
			temp |= (value & 2) << 3;
			temp |= (value & 1) << 5;
			
			temp = value & 3;			// Drive Select
			temp |= (value & 4) << 2;	// Side Select
			temp |= (value & 8) << 2;	// Density
			
			break;
	}
	
	WriteFDCControlReg(temp);
}

unsigned char BeebWin::GetDriveControl(void)
{
	
	unsigned char value;
	unsigned char temp;

	temp = 0;
	
	value = ReadFDCControlReg();
	
	switch (FDCType)
	{
		case 0 :		// Acorn
			temp = value & 3;
			temp |= (value & 48) >> 2;
			break;
		case 1 :		// Opus
			temp = ((value ^ 255) & 32) << 1;
			temp |= (value & 16) >> 3;
			if (value & 2) temp |= 1;
			break;
		case 2 :		// Watford
			if (value & 1) temp = 0;
			if (value & 2) temp = 4;
			temp |= (value & 16) >> 3;
			temp |= (value & 32) >> 5;
			
			temp = value & 3;
			temp |= (value & 16) >> 2;
			temp |= (value & 32) >> 2;
			
			break;
	}
	
	return temp;
}


/****************************************************************************/
void BeebWin::TranslateCapture(void)
{
	
	SetMenuCommandIDCheck('skp0', false);
	SetMenuCommandIDCheck('skp1', false);
	SetMenuCommandIDCheck('skp2', false);
	SetMenuCommandIDCheck('skp3', false);
	SetMenuCommandIDCheck('skp4', false);
	SetMenuCommandIDCheck('skp5', false);
	SetMenuCommandIDCheck('rec1', false);
	SetMenuCommandIDCheck('rec2', false);
	SetMenuCommandIDCheck('rec3', false);
	SetMenuCommandIDCheck('rec4', false);
	
	switch (m_skip)
	{
		case 0:
			SetMenuCommandIDCheck('skp0', true);
			break;
		case 1:
			SetMenuCommandIDCheck('skp1', true);
			break;
		case 2:
			SetMenuCommandIDCheck('skp2', true);
			break;
		case 3:
			SetMenuCommandIDCheck('skp3', true);
			break;
		case 4:
			SetMenuCommandIDCheck('skp4', true);
			break;
		case 5:
			SetMenuCommandIDCheck('skp5', true);
			break;
	}

	switch (m_captureresolution)
	{
		case 1:
			SetMenuCommandIDCheck('rec1', true);
			break;
		case 2:
			SetMenuCommandIDCheck('rec2', true);
			break;
		case 3:
			SetMenuCommandIDCheck('rec3', true);
			break;
		case 4:
			SetMenuCommandIDCheck('rec4', true);
			break;
	}
	
}

/****************************************************************************/
void BeebWin::TranslateSampleRate(void)
{
// IDM_44100KHZ:
//	SoundSampleRate = 44100;

// IDM_22050KHZ:
//	SoundSampleRate = 22050;
	
// IDM_11025KHZ:
//	SoundSampleRate = 11025;
}

/****************************************************************************/
void BeebWin::TranslateWindowSize(int size)
{

	if ( (size < 0) || (size > 10) ) return;
	
	SetMenuCommandIDCheck('siz1', false);
	SetMenuCommandIDCheck('siz2', false);
	SetMenuCommandIDCheck('siz3', false);
	SetMenuCommandIDCheck('siz4', false);
	SetMenuCommandIDCheck('siz5', false);
	SetMenuCommandIDCheck('siz6', false);
	SetMenuCommandIDCheck('siz7', false);
	SetMenuCommandIDCheck('siz8', false);
	SetMenuCommandIDCheck('siz9', false);
	SetMenuCommandIDCheck('siza', false);
	SetMenuCommandIDCheck('sizb', false);

	m_MenuIdWinSize = size;

	switch (size)
	{
	case 0 :
		SetMenuCommandIDCheck('siz1', true);
		m_XWinSize = 160;
		m_YWinSize = 128;
		break;
	case 1 :
		SetMenuCommandIDCheck('siz2', true);
		m_XWinSize = 240;
		m_YWinSize = 192;
		break;
	case 2 :
		SetMenuCommandIDCheck('siz3', true);
		m_XWinSize = 320;
		m_YWinSize = 256;
		break;
	case 3 :
		SetMenuCommandIDCheck('siz4', true);
		m_XWinSize = 640;
		m_YWinSize = 256;
		break;
	case 4 :
		SetMenuCommandIDCheck('siz5', true);
		m_XWinSize = 640;
		m_YWinSize = 512;
		break;
	case 5 :
		SetMenuCommandIDCheck('siz6', true);
		m_XWinSize = 800;
		m_YWinSize = 600;
		break;
	case 6 :
		SetMenuCommandIDCheck('siz7', true);
		m_XWinSize = 1024;
		m_YWinSize = 512;
		break;
	case 7 :
		SetMenuCommandIDCheck('siz8', true);
		m_XWinSize = 1024;
		m_YWinSize = 768;
		break;
	case 8 :
		SetMenuCommandIDCheck('siz9', true);
		m_XWinSize = 1280;
		m_YWinSize = 1024;
		break;
	case 9 :
		SetMenuCommandIDCheck('siza', true);
		m_XWinSize = 1440;
		m_YWinSize = 1080;
		break;
	case 10 :
		SetMenuCommandIDCheck('sizb', true);
		m_XWinSize = 1600;
		m_YWinSize = 1200;
		break;
	}

	SizeWindow(mWindow, m_XWinSize, m_YWinSize, false);
	RepositionWindow(mWindow, NULL, kWindowCenterOnMainScreen);

}

void BeebWin::doLED(int sx,bool on) {
	int tsy; char colbase;
	colbase=(DiscLedColour*2)+LED_COL_BASE; // colour will be 0 for red, 1 for green.
	if (sx<100) colbase=LED_COL_BASE; // Red leds for keyboard always
//	if (TeletextEnabled) tsy=496; else tsy=254;
	if (TeletextEnabled) tsy=500; else tsy=286;
	doUHorizLine(mainWin->cols[((on)?1:0)+colbase],tsy,sx,8);
	doUHorizLine(mainWin->cols[((on)?1:0)+colbase],tsy,sx,8);
};
 
void BeebWin::ReadDisc(int drive)

{
OSErr err = noErr;
char path[256];

	err = GetOneFileWithPreview(path, DiscFilterProc);
	if (err) return;
	LoadDisc(drive, path);
}

void BeebWin::LoadTape()
{
	OSErr err = noErr;
	char path[256];
	
	err = GetOneFileWithPreview(path, UEFFilterProc);
	if (err) return;
	LoadTapeFromPath(path);
}

void BeebWin::LoadTapeFromPath(char *path)
{
	if (strstr(path, ".uef")) LoadUEF(path);
	if (strstr(path, ".csw")) LoadCSW(path);
}

void BeebWin::LoadDisc(int drive, char *path)
{
bool dsd = false;
bool adfs = false;
bool img = false;
bool dos = false;
bool ssd = false;
bool wdd = false;

	if (strstr(path, ".ssd")) ssd = true;
	if (strstr(path, ".dsd")) dsd = true;
	if (strstr(path, ".wdd")) wdd = true;
	if (strstr(path, ".adl")) adfs = true;
	if (strstr(path, ".adf")) adfs = true;
	if (strstr(path, ".img")) img = true;
	if (strstr(path, ".dos")) dos = true;
	if (strstr(path, ".SSD")) ssd = true;
	if (strstr(path, ".DSD")) dsd = true;
	if (strstr(path, ".WDD")) wdd = true;
	if (strstr(path, ".ADL")) adfs = true;
	if (strstr(path, ".ADF")) adfs = true;
	if (strstr(path, ".IMG")) img = true;
	if (strstr(path, ".DOS")) dos = true;
	
	if (MachineType != 3)
	{
		if (dsd)
		{
			if (NativeFDC)
				LoadSimpleDSDiscImage(path, drive, 80);
			else
				Load1770DiscImage(path, drive, 1);		// 1 = dsd
		}
		if (ssd || img)
		{
			if (NativeFDC)
				LoadSimpleDiscImage(path,drive, 0, 80);
			else
				Load1770DiscImage(path, drive, 0);		// 0 = ssd
		}
		if (adfs)
		{
			if (NativeFDC)
			{
				fprintf(stderr, "The native 8271 FDC cannot read ADFS discs\n");
			}
			else
				Load1770DiscImage(path, drive, 2);					// 2 = adfs
		}
		if (wdd)
		{
			if (NativeFDC)
			{
				fprintf(stderr, "The native 8271 FDC cannot read Watford Double Density discs\n");
			}
			else
				Load1770DiscImage(path, drive, 5);					// 5 = watford double density
		}
	}
			
	if (MachineType == 3)
	{
		if (dsd)
			Load1770DiscImage(path, drive, 1);						// 1 = dsd
		if (ssd)
			Load1770DiscImage(path, drive, 0);						// 0 = ssd
		if (adfs)
			Load1770DiscImage(path, drive, 2);						// ADFS
		if (img)
			Load1770DiscImage(path, drive, 3);						// 800K DOS PLUS
		if (dos)
			Load1770DiscImage(path, drive, 4);						// 720K DOS PLUS
		if (wdd)
			Load1770DiscImage(path, drive, 5);						// 720K WATFORD DOUBLE DENSITY
	}
						
	if (m_WriteProtectOnLoad != m_WriteProtectDisc[drive])
		ToggleWriteProtect(drive);
}

/****************************************************************************/
bool BeebWin::UpdateTiming(void)
{
	unsigned long TickCount;
	unsigned long Ticks;
	unsigned long SpareTicks;
	int Cycles;
	int CyclesPerSec;
	bool UpdateScreen = FALSE;
	static int firsttime = 0;

	TickCount = GetTickCount();

	/* Don't do anything if this is the first call or there has
	   been a long pause due to menu commands, or when something
	   wraps. */

	if ( (m_LastTickCount == 0) ||
		 (TickCount < m_LastTickCount) ||
		((TickCount - m_LastTickCount) > 1000) ||
		(firsttime == 0) ||
		(TotalCycles < m_LastTotalCycles) )
	{
		firsttime = 1;
		ResetTiming();
		return TRUE;
	}

	/* Update stats every second */
	if (TickCount >= m_LastStatsTickCount + 1000)
	{

		m_FramesPerSecond = m_ScreenRefreshCount;
		m_ScreenRefreshCount = 0;
		m_RelativeSpeed = ((TotalCycles - m_LastStatsTotalCycles) / 2000.0) /
								(TickCount - m_LastStatsTickCount);

		m_LastStatsTotalCycles = TotalCycles;
		m_LastStatsTickCount += 1000;
		DisplayTiming();
		
	}

	// Now we work out if BeebEm is running too fast or not
	if (m_RealTimeTarget > 0.0)
	{
		Ticks = TickCount - m_TickBase;
		Cycles = (int)((double)(TotalCycles - m_CycleBase) / m_RealTimeTarget);

		if (Ticks <= (unsigned long)(Cycles / 2000))
		{
			// Need to slow down, show frame (max 50fps though) 
			// and sleep a bit

			if (TickCount >= m_LastFPSCount + 20)
			{
				UpdateScreen = TRUE;
				m_LastFPSCount += 20;
			}
			else
			{
				UpdateScreen = FALSE;
			}

			SpareTicks = ((unsigned long)(Cycles / 2000) - Ticks) * 1000;
	
			usleep( SpareTicks + 500);

			m_MinFrameCount = 0;
		}
		else
		{

			// Need to speed up, skip a frame
			UpdateScreen = FALSE;

			// Make sure we show at least one in 100 frames
			++m_MinFrameCount;
			if (m_MinFrameCount >= 100)
			{
				UpdateScreen = TRUE;
				m_MinFrameCount = 0;
			}
		}

		/* Move counter bases forward */
		CyclesPerSec = (int) (2000000.0 * m_RealTimeTarget);

		while ( ((TickCount - m_TickBase) > 1000) && ((TotalCycles - m_CycleBase) > CyclesPerSec))
		{
			m_TickBase += 1000;
			m_CycleBase += CyclesPerSec;
		}
	}
	else
	{
		/* Fast as possible with a certain frame rate */
		if (TickCount >= m_LastFPSCount + (1000 / m_FPSTarget))
		{
			UpdateScreen = TRUE;
			m_LastFPSCount += (1000 / m_FPSTarget);
		}
		else
		{
			UpdateScreen = FALSE;
		}
	}

	m_LastTickCount = TickCount;
	m_LastTotalCycles = TotalCycles;

	return UpdateScreen;
}

/****************************************************************************/
void BeebWin::DisplayTiming(void)
{
CFStringRef pTitle;

	if (m_ShowSpeedAndFPS && !m_isFullScreen)
	{
		sprintf(m_szTitle, "%s  Speed: %2.2f  fps: %2d",
				WindowTitle, m_RelativeSpeed, (int)m_FramesPerSecond);

		pTitle = CFStringCreateWithCString (kCFAllocatorDefault, m_szTitle, kCFStringEncodingASCII);

		SetWindowTitleWithCFString(mWindow, pTitle);
		
		CFRelease(pTitle);

	}
}

/****************************************************************************/
unsigned long BeebWin::GetTickCount(void)
{
  struct timeval now;
  struct timezone tpz;
  long milli;
	
  gettimeofday(&now, &tpz);

  now.tv_sec %= (60 * 60 * 24);

  milli = (now.tv_sec % (60 * 60 * 24)) * 1000L + now.tv_usec / 1000L;

  return milli;
}

/****************************************************************************/
int BeebWin::StartOfFrame(void)
{
	int FrameNum = 1;

	if (UpdateTiming())
		FrameNum = 0;

	return FrameNum;
}

void BeebWin::ToggleWriteProtect(int Drive)
{

	if (MachineType != 3)
	{
		if (m_WriteProtectDisc[Drive])
		{
			m_WriteProtectDisc[Drive] = 0;
			DiscWriteEnable(Drive, 1);
		}
		else
		{
			m_WriteProtectDisc[Drive] = 1;
			DiscWriteEnable(Drive, 0);
		}
	
		if (Drive == 0)
			SetMenuCommandIDCheck('wrp0', (m_WriteProtectDisc[0]) ? true : false);
		else
			SetMenuCommandIDCheck('wrp1', (m_WriteProtectDisc[1]) ? true : false);
	}

	if ((MachineType == 3) || ((MachineType == 0) && (!NativeFDC)))
	{
		DWriteable[Drive] = 1 - DWriteable[Drive];

		fprintf(stderr, "Drive = %d, Writeable = %d\n", Drive, DWriteable[Drive]);
		
		if (Drive == 0)
			SetMenuCommandIDCheck('wrp0', (DWriteable[0]) ? false : true);
		else
			SetMenuCommandIDCheck('wrp1', (DWriteable[1]) ? false : true);
	}
}

void BeebWin::SetDiscWriteProtects(void)
{

	if (MachineType != 3)
	{
		m_WriteProtectDisc[0] = !IsDiscWritable(0);
		m_WriteProtectDisc[1] = !IsDiscWritable(1);
		SetMenuCommandIDCheck('wrp0', (m_WriteProtectDisc[0]) ? true : false);
		SetMenuCommandIDCheck('wrp1', (m_WriteProtectDisc[1]) ? true : false);
	}
	else
	{
		SetMenuCommandIDCheck('wrp0', (DWriteable[0]) ? false : true);
		SetMenuCommandIDCheck('wrp1', (DWriteable[1]) ? false : true);
	}
}

void BeebWin::InitMenu(void)
{

	SetMenuCommandIDCheck('ofwm', (m_FreezeWhenInactive) ? true : false);
	SetMenuCommandIDCheck('msea', (m_MenuIdSticks == 1) ? true : false);
	SetMenuCommandIDCheck('msed', (m_MenuIdSticks == 2) ? true : false);

	SetMenuCommandIDCheck('kmas', (m_KeyMapAS) ? true : false);

	SetMenuCommandIDCheck('docu', (OpCodes == 1) ? true : false);
	SetMenuCommandIDCheck('extr', (OpCodes == 2) ? true : false);
	SetMenuCommandIDCheck('full', (OpCodes == 3) ? true : false);

	SetMenuCommandIDCheck('hard', (BHardware) ? true : false);
	SetMenuCommandIDCheck('igil', (IgnoreIllegalInstructions) ? true : false);
	SetMenuCommandIDCheck('sfps', (m_ShowSpeedAndFPS) ? true : false);
	SetMenuCommandIDCheck('sped', (SpeechEnabled) ? true : false);
	SetMenuCommandIDCheck('sond', (SoundEnabled) ? true : false);
	SetMenuCommandIDCheck('sndc', (SoundChipEnabled) ? true : false);
	SetMenuCommandIDCheck('snev', (SoundExponentialVolume) ? true : false);
	SetMenuCommandIDCheck('tube', (TubeEnabled) ? true : false);
	SetMenuCommandIDCheck('t186', (Tube186Enabled) ? true : false);
	SetMenuCommandIDCheck('tz80', (TorchTube) ? true : false);
	SetMenuCommandIDCheck('tarm', (ArmTube) ? true : false);
	SetMenuCommandIDCheck('az80', (AcornZ80) ? true : false);
	SetMenuCommandIDCheck('txte', (TeleTextAdapterEnabled) ? true : false);
	SetMenuCommandIDCheck('txtd', (TeleTextData) ? true : false);
	SetMenuCommandIDCheck('txts', (TeleTextServer) ? true : false);
	SetMenuCommandIDCheck('hdre', (HardDriveEnabled) ? true : false);
	SetMenuCommandIDCheck('invb', (m_Invert) ? true : false);
	
	SetMenuCommandIDCheck('tpso', TapeSoundEnabled ? true : false);
	SetMenuCommandIDCheck('tpcr', RelaySoundEnabled ? true : false);
	SetMenuCommandIDCheck('ddso', DiscDriveSoundEnabled ? true : false);

	SetMenuCommandIDCheck('tpul', UnlockTape ? true : false);
	
	SetMenuCommandIDCheck('prnt', (PrinterEnabled) ? true : false);
	SetMenuCommandIDCheck('rs42', (SerialPortEnabled) ? true : false);
	SetMenuCommandIDCheck('sdts', (TouchScreenEnabled) ? true : false);
	SetMenuCommandIDCheck('sdep', (EthernetPortEnabled) ? true : false);
	SetMenuCommandIDCheck('uprm', (RTC_Enabled) ? true : false);

	SetMenuCommandIDCheck('wpol', (m_WriteProtectOnLoad) ? true : false);
	SetMenuCommandIDCheck('vmar', (m_maintainAspectRatio) ? true : false);

	char Title[256];

	MenuRef			menu = nil;
	MenuItemIndex	j;
	OSStatus		err;

	err = GetIndMenuItemWithCommandID(nil, 'pfle', 1, &menu, &j);
	if (!err)
	{
		CFStringRef pTitle;
		sprintf(Title, "File: %s", m_PrinterFileName);
		pTitle = CFStringCreateWithCString (kCFAllocatorDefault, Title, kCFStringEncodingASCII);
		err = SetMenuItemTextWithCFString(menu, j, pTitle);
		if (err) fprintf(stderr, "SetMenuTitle returned err code %d\n", (int) err);
		CFRelease(pTitle);
	}

	if (MachineType != 3)
	{
		SetMenuCommandIDCheck('wrp0', (m_WriteProtectDisc[0]) ? true : false);
		SetMenuCommandIDCheck('wrp1', (m_WriteProtectDisc[1]) ? true : false);
	}
	else
	{
		SetMenuCommandIDCheck('wrp0', (DWriteable[0]) ? false : true);
		SetMenuCommandIDCheck('wrp1', (DWriteable[1]) ? false : true);
	}
	
	SetMenuCommandIDCheck('dkm ', (m_MenuIdKeyMapping == 0) ? true : false);
	SetMenuCommandIDCheck('udkm', (m_MenuIdKeyMapping == 1) ? true : false);

	UpdateEconetMenu();
	UpdateMonitorMenu();
	UpdateModelType();
	UpdateLEDMenu();
	UpdateMotionBlurMenu();
	UpdateAMXMenu();
	SetTapeSpeedMenu();

	SetRomMenu();
}

void BeebWin::UpdateAMXMenu(void)
{
	SetMenuCommandIDCheck('amxo', (AMXMouseEnabled) ? true : false);
	SetMenuCommandIDCheck('amxl', (AMXLRForMiddle) ? true : false);

	SetMenuCommandIDCheck('amx1', false);
	SetMenuCommandIDCheck('amx3', false);
	SetMenuCommandIDCheck('amx6', false);
	SetMenuCommandIDCheck('axp5', false);
	SetMenuCommandIDCheck('axp3', false);
	SetMenuCommandIDCheck('axp1', false);
	SetMenuCommandIDCheck('axm1', false);
	SetMenuCommandIDCheck('axm3', false);
	SetMenuCommandIDCheck('axm5', false);

	if (m_MenuIdAMXSize == 1) SetMenuCommandIDCheck('amx1', true);
	if (m_MenuIdAMXSize == 2) SetMenuCommandIDCheck('amx3', true);
	if (m_MenuIdAMXSize == 3) SetMenuCommandIDCheck('amx6', true);

	if (m_MenuIdAMXAdjust == 1) SetMenuCommandIDCheck('axp5', true);
	if (m_MenuIdAMXAdjust == 2) SetMenuCommandIDCheck('axp3', true);
	if (m_MenuIdAMXAdjust == 3) SetMenuCommandIDCheck('axp1', true);
	if (m_MenuIdAMXAdjust == 4) SetMenuCommandIDCheck('axm1', true);
	if (m_MenuIdAMXAdjust == 5) SetMenuCommandIDCheck('axm3', true);
	if (m_MenuIdAMXAdjust == 6) SetMenuCommandIDCheck('axm5', true);
}

void BeebWin::UpdateMotionBlurMenu(void)
{
	SetMenuCommandIDCheck('mbof', (m_Motion_Blur == 0) ? true : false);
	SetMenuCommandIDCheck('mb2f', (m_Motion_Blur == 1) ? true : false);
	SetMenuCommandIDCheck('mb4f', (m_Motion_Blur == 2) ? true : false);
	SetMenuCommandIDCheck('mb8f', (m_Motion_Blur == 3) ? true : false);
}

void BeebWin::UpdateLEDMenu(void)
{
	SetMenuCommandIDCheck('ledr', (DiscLedColour == 0) ? true : false);
	SetMenuCommandIDCheck('ledg', (DiscLedColour == 1) ? true : false);
	SetMenuCommandIDCheck('ledk', (LEDs.ShowKB) ? true : false);
	SetMenuCommandIDCheck('ledd', (LEDs.ShowDisc) ? true : false);
}

void BeebWin::UpdateMonitorMenu(void)
{
	SetMenuCommandIDCheck('monr', (palette_type == RGB) ? true : false);
	SetMenuCommandIDCheck('monb', (palette_type == BW) ? true : false);
	SetMenuCommandIDCheck('mong', (palette_type == GREEN) ? true : false);
	SetMenuCommandIDCheck('mona', (palette_type == AMBER) ? true : false);
}

void BeebWin::UpdateModelType(void)
{
	SetMenuCommandIDCheck('bbcb', (MachineType == 0) ? true : false);
	SetMenuCommandIDCheck('bbci', (MachineType == 1) ? true : false);
	SetMenuCommandIDCheck('bbcp', (MachineType == 2) ? true : false);
	SetMenuCommandIDCheck('bbcm', (MachineType == 3) ? true : false);
}

void BeebWin::UpdateEconetMenu(void)
{
	SetMenuCommandIDCheck('enet', (EconetEnabled == 1) ? true : false);
}

void BeebWin::SetTapeSpeedMenu(void)
{
	SetMenuCommandIDCheck('tpfa', (TapeClockSpeed == 750) ? true : false);
	SetMenuCommandIDCheck('tpmf', (TapeClockSpeed == 1600) ? true : false);
	SetMenuCommandIDCheck('tpms', (TapeClockSpeed == 3200) ? true : false);
	SetMenuCommandIDCheck('tpno', (TapeClockSpeed == 5600) ? true : false);
}

void BeebWin::SetMenuCommandIDCheck(UInt32 commandID, bool check)
{
	MenuRef			menu = nil;
	MenuItemIndex	i;
	OSStatus		err;
	
	err = GetIndMenuItemWithCommandID(nil, commandID, 1, &menu, &i);
	if (!err)
	{
		CheckMenuItem(menu,i,check);
	}
	else
	{
		fprintf(stderr, "Cannot find menu id %08x\n", (unsigned int) commandID);
	}
}

void BeebWin::UpdatePalette(PaletteType NewPal)
{
int col;
float r, g, b;

  palette_type = NewPal;

  m_BlurIntensities[0] = 100;
  m_BlurIntensities[1] = 88;
  m_BlurIntensities[2] = 75;
  m_BlurIntensities[3] = 62;
  m_BlurIntensities[4] = 50;
  m_BlurIntensities[5] = 38;
  m_BlurIntensities[6] = 25;
  m_BlurIntensities[7] = 12;
  
  for(col = 0; col < 64; col++)
  {
	cols[col] = col;
    mCT->ctTable[col].value = col;

	r = (col & 1);
	g = (col & 2) >> 1;
	b = (col & 4) >> 2;
	
	if (palette_type != RGB)
	{
		r = g = b = 0.299 * r + 0.587 * g + 0.114 * b;
		switch (palette_type)
		{
		case AMBER:
			r *= 1.0;
			g *= 0.8;
			b *= 0.1;
			break;
		case GREEN:
			r *= 0.2;
			g *= 0.9;
			b *= 0.1;
			break;
		}
	}
	
    mCT->ctTable[col].rgb.red = (int) (r * m_BlurIntensities[col >> 3] / 100.0 * 0xffff);
    mCT->ctTable[col].rgb.green = (int) (g * m_BlurIntensities[col >> 3] / 100.0 * 0xffff);
    mCT->ctTable[col].rgb.blue = (int) (b * m_BlurIntensities[col >> 3] / 100.0 * 0xffff);

  };

// Red Leds - left is dark, right is lit

  mCT->ctTable[LED_COL_BASE].value = LED_COL_BASE;
  mCT->ctTable[LED_COL_BASE].rgb.red = 80 * 256;
  mCT->ctTable[LED_COL_BASE].rgb.green = 0;
  mCT->ctTable[LED_COL_BASE].rgb.blue = 0;

  mCT->ctTable[LED_COL_BASE + 1].value = LED_COL_BASE + 1;
  mCT->ctTable[LED_COL_BASE + 1].rgb.red = 0xffff;
  mCT->ctTable[LED_COL_BASE + 1].rgb.green = 0;
  mCT->ctTable[LED_COL_BASE + 1].rgb.blue = 0;

// Green Leds - left is dark, right is lit

  mCT->ctTable[LED_COL_BASE + 2].value = LED_COL_BASE + 2;
  mCT->ctTable[LED_COL_BASE + 2].rgb.red = 0;
  mCT->ctTable[LED_COL_BASE + 2].rgb.green = 80 * 256;
  mCT->ctTable[LED_COL_BASE + 2].rgb.blue = 0;

  mCT->ctTable[LED_COL_BASE + 3].value = LED_COL_BASE + 3;
  mCT->ctTable[LED_COL_BASE + 3].rgb.red = 0;
  mCT->ctTable[LED_COL_BASE + 3].rgb.green = 0xffff;
  mCT->ctTable[LED_COL_BASE + 3].rgb.blue = 0;

  mBitMap.pmTable = &mCT;

PaletteHandle gPalette;

  gPalette = NewPalette( LED_COL_BASE + 4,  &mCT, pmExplicit + pmTolerant, 0 );

  SetPortWindowPort(mWindow);
  SetPalette (mWindow, gPalette, true);


  CTabChanged(mBitMap.pmTable);

  UpdateMonitorMenu();

  for (int i = 0; i < LED_COL_BASE + 4; ++i)
  {
	m_RGB32[i] = ((( ((mCT->ctTable[i].rgb.red >> 8) << 8)  + (mCT->ctTable[i].rgb.green >> 8)) << 8) + (mCT->ctTable[i].rgb.blue >> 8));
	m_RGB32[i] |= 0xff000000;

	m_RGB16[i] = ((( ((mCT->ctTable[i].rgb.red >> 11) << 5)  + (mCT->ctTable[i].rgb.green >> 11)) << 5) + (mCT->ctTable[i].rgb.blue >> 11));

//	WriteLog("RGB32[%d] = %08x, RGB16[%d] = %04x\n", i, m_RGB32[i], i, m_RGB16[i]);
  
  }

}

OSStatus TCWindowCommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData);

OSStatus BeebWin::HandleCommand(UInt32 cmdID)

{
int i;
OSStatus err = noErr;

//	fprintf(stderr, "Command ID = %08x\n", cmdID);

	switch (cmdID)
    {
        case 'sfps':
            fprintf(stderr, "Show Speed and FPS selected\n");
			if (m_ShowSpeedAndFPS)
			{
				m_ShowSpeedAndFPS = false;
				SetMenuCommandIDCheck('sfps', false);

				CFStringRef pTitle;
				pTitle = CFStringCreateWithCString (kCFAllocatorDefault, WindowTitle, kCFStringEncodingASCII);
				SetWindowTitleWithCFString(mWindow, pTitle);
				CFRelease(pTitle);
			}
			else
			{
				m_ShowSpeedAndFPS = true;
				SetMenuCommandIDCheck('sfps', true);
			}
	
            break;

        case 'rund':
            fprintf(stderr, "Run Disc selected\n");
			ReadDisc(0);
			m_ShiftBooted = true;
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			BeebKeyDown(0, 0);
            break;
        case 'opn0':
            fprintf(stderr, "Load Disc 0 selected\n");
			ReadDisc(0);
            break;
        case 'opn1':
            fprintf(stderr, "Load Disc 1 selected\n");
			ReadDisc(1);
            break;
        case 'new0':
            fprintf(stderr, "New Disc 0 selected\n");
			NewDiscImage(0);
            break;
        case 'new1':
            fprintf(stderr, "New Disc 1 selected\n");
			NewDiscImage(1);
            break;
        case 'rest':
            fprintf(stderr, "Reset selected\n");
			ResetBeebSystem(MachineType, TubeEnabled, 0);
            break;
        case 'bbcb':
            fprintf(stderr, "BBC B selected\n");
			if (MachineType != 0)
			{
				ResetBeebSystem(0, EnableTube, 1);
				UpdateModelType();
			}
            break;
        case 'bbci':
            fprintf(stderr, "BBC B Integra selected\n");
			if (MachineType != 1)
			{
				ResetBeebSystem(1, EnableTube, 1);
				UpdateModelType();
			}
            break;
        case 'bbcp':
            fprintf(stderr, "BBC B Plus selected\n");
			if (MachineType != 2)
			{
				ResetBeebSystem(2, EnableTube, 1);
				UpdateModelType();
			}
            break;
        case 'bbcm':
            fprintf(stderr, "BBC Master 128 selected\n");
			if (MachineType != 3)
			{
				ResetBeebSystem(3, EnableTube, 1);
				UpdateModelType();
			}
            break;
        case 'tube':
            fprintf(stderr, "Tube selected\n");
			TubeEnabled = 1 - TubeEnabled;
			Tube186Enabled = 0;
			TorchTube = 0;
			AcornZ80 = 0;
			ArmTube = 0;
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			SetMenuCommandIDCheck('tube', (TubeEnabled) ? true : false);
			SetMenuCommandIDCheck('t186', (Tube186Enabled) ? true : false);
			SetMenuCommandIDCheck('tarm', (ArmTube) ? true : false);
			SetMenuCommandIDCheck('tz80', (TorchTube) ? true : false);
			SetMenuCommandIDCheck('az80', (AcornZ80) ? true : false);
            break;
        case 't186':
            fprintf(stderr, "Tube 80186 selected\n");
			Tube186Enabled = 1 - Tube186Enabled;
			TubeEnabled = 0;
			TorchTube = 0;
			ArmTube = 0;
			AcornZ80 = 0;
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			SetMenuCommandIDCheck('tube', (TubeEnabled) ? true : false);
			SetMenuCommandIDCheck('t186', (Tube186Enabled) ? true : false);
			SetMenuCommandIDCheck('tarm', (ArmTube) ? true : false);
			SetMenuCommandIDCheck('tz80', (TorchTube) ? true : false);
			SetMenuCommandIDCheck('az80', (AcornZ80) ? true : false);
            break;
        case 'tz80':
            fprintf(stderr, "Torch Z80 Tube selected\n");
			TorchTube = 1 - TorchTube;
			Tube186Enabled = 0;
			TubeEnabled = 0;
			ArmTube = 0;
			AcornZ80 = 0;
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			SetMenuCommandIDCheck('tube', (TubeEnabled) ? true : false);
			SetMenuCommandIDCheck('t186', (Tube186Enabled) ? true : false);
			SetMenuCommandIDCheck('tarm', (ArmTube) ? true : false);
			SetMenuCommandIDCheck('tz80', (TorchTube) ? true : false);
			SetMenuCommandIDCheck('az80', (AcornZ80) ? true : false);
            break;
        case 'tarm':
            fprintf(stderr, "Arm Tube selected\n");
			ArmTube = 1 - ArmTube;
			Tube186Enabled = 0;
			TubeEnabled = 0;
			AcornZ80 = 0;
			TorchTube = 0;
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			SetMenuCommandIDCheck('tube', (TubeEnabled) ? true : false);
			SetMenuCommandIDCheck('t186', (Tube186Enabled) ? true : false);
			SetMenuCommandIDCheck('tarm', (ArmTube) ? true : false);
			SetMenuCommandIDCheck('tz80', (TorchTube) ? true : false);
			SetMenuCommandIDCheck('az80', (AcornZ80) ? true : false);
            break;
        case 'az80':
            fprintf(stderr, "Acorn Z80 Tube selected\n");
			AcornZ80 = 1 - AcornZ80;
			Tube186Enabled = 0;
			TubeEnabled = 0;
			TorchTube = 0;
			ArmTube = 0;
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			SetMenuCommandIDCheck('tube', (TubeEnabled) ? true : false);
			SetMenuCommandIDCheck('t186', (Tube186Enabled) ? true : false);
			SetMenuCommandIDCheck('tarm', (ArmTube) ? true : false);
			SetMenuCommandIDCheck('tz80', (TorchTube) ? true : false);
			SetMenuCommandIDCheck('az80', (AcornZ80) ? true : false);
            break;
			
        case 'sRT ':
            fprintf(stderr, "Real Time selected\n");
			TranslateTiming(IDM_REALTIME);
            break;

        case 's50 ':
            fprintf(stderr, "50 FPS selected\n");
			TranslateTiming(IDM_50FPS);
            break;

        case 's25 ':
            fprintf(stderr, "25 FPS selected\n");
			TranslateTiming(IDM_25FPS);
            break;
        case 's10 ':
            fprintf(stderr, "10 FPS selected\n");
			TranslateTiming(IDM_10FPS);
            break;
        case 's5  ':
            fprintf(stderr, "5 FPS selected\n");
			TranslateTiming(IDM_5FPS);
            break;
        case 's1  ':
            fprintf(stderr, "1 FPS selected\n");
			TranslateTiming(IDM_1FPS);
            break;

        case 'f100':
            fprintf(stderr, "Fixed Speed 100\n");
			TranslateTiming(IDM_FIXEDSPEED100);
            break;

        case 'f50 ':
            fprintf(stderr, "Fixed Speed 50\n");
			TranslateTiming(IDM_FIXEDSPEED50);
            break;

        case 'f10 ':
            fprintf(stderr, "Fixed Speed 10\n");
			TranslateTiming(IDM_FIXEDSPEED10);
            break;

        case 'f5  ':
            fprintf(stderr, "Fixed Speed 5\n");
			TranslateTiming(IDM_FIXEDSPEED5);
            break;

        case 'f2  ':
            fprintf(stderr, "Fixed Speed 2\n");
			TranslateTiming(IDM_FIXEDSPEED2);
            break;
        case 'f1.5':
            fprintf(stderr, "Fixed Speed 1.5\n");
			TranslateTiming(IDM_FIXEDSPEED1_5);
            break;
        case 'f1.2':
            fprintf(stderr, "Fixed Speed 1.25\n");
			TranslateTiming(IDM_FIXEDSPEED1_25);
            break;
        case 'f1.1':
            fprintf(stderr, "Fixed Speed 1.1\n");
			TranslateTiming(IDM_FIXEDSPEED1_1);
            break;
        case 'f0.9':
            fprintf(stderr, "Fixed Speed 0.9\n");
			TranslateTiming(IDM_FIXEDSPEED0_9);
            break;
        case 'f0.7':
            fprintf(stderr, "Fixed Speed 0.75\n");
			TranslateTiming(IDM_FIXEDSPEED0_75);
            break;
        case 'f0.5':
            fprintf(stderr, "Fixed Speed 0.5\n");
			TranslateTiming(IDM_FIXEDSPEED0_5);
            break;
        case 'f0.2':
            fprintf(stderr, "Fixed Speed 0.25\n");
			TranslateTiming(IDM_FIXEDSPEED0_25);
            break;
        case 'f0.1':
            fprintf(stderr, "Fixed Speed 0.1\n");
			TranslateTiming(IDM_FIXEDSPEED0_1);
            break;

// monitor type

        case 'monr':
            fprintf(stderr, "RGB selected\n");
			UpdatePalette(RGB);
            break;

        case 'monb':
            fprintf(stderr, "BW selected\n");
			UpdatePalette(BW);
            break;

        case 'mong':
            fprintf(stderr, "GREEN selected\n");
			UpdatePalette(GREEN);
            break;

        case 'mona':
            fprintf(stderr, "AMBER selected\n");
			UpdatePalette(AMBER);
            break;

// disc write protects

        case 'wrp0':
            fprintf(stderr, "Toggle Write Protect Disc 0 selected\n");
			ToggleWriteProtect(0);
            break;

        case 'wrp1':
            fprintf(stderr, "Toggle Write Protect Disc 1 selected\n");
			ToggleWriteProtect(1);
            break;

		case 'wpol':
			fprintf(stderr, "Protect on load selected\n");
			m_WriteProtectOnLoad = 1 - m_WriteProtectOnLoad;
			SetMenuCommandIDCheck('wpol', (m_WriteProtectOnLoad) ? true : false);
            break;
			
// screen size

        case 'siz1':
            fprintf(stderr, "160x128 selected\n");
			TranslateWindowSize(0);
            break;

        case 'siz2':
            fprintf(stderr, "240x192 selected\n");
			TranslateWindowSize(1);
            break;

        case 'siz3':
            fprintf(stderr, "320x256 selected\n");
			TranslateWindowSize(2);
            break;

        case 'siz4':
            fprintf(stderr, "640x256 selected\n");
			TranslateWindowSize(3);
            break;

        case 'siz5':
            fprintf(stderr, "640x512 selected\n");
			TranslateWindowSize(4);
            break;

        case 'siz6':
            fprintf(stderr, "800x600 selected\n");
			TranslateWindowSize(5);
            break;

        case 'siz7':
            fprintf(stderr, "1024x512 selected\n");
			TranslateWindowSize(6);
            break;

        case 'siz8':
            fprintf(stderr, "1024x768 selected\n");
			TranslateWindowSize(7);
            break;

		case 'siz9':
			fprintf(stderr, "1280x1024 selected\n");
			TranslateWindowSize(8);
			break;

		case 'siza':
			fprintf(stderr, "1440x1080 selected\n");
			TranslateWindowSize(9);
			break;
			
		case 'sizb':
			fprintf(stderr, "1600x1200 selected\n");
			TranslateWindowSize(10);
			break;
			
		case 'vfsc':
            fprintf(stderr, "View full screen selected %d, %d\n", m_XWinSize, m_YWinSize);
			m_isFullScreen = 1 - m_isFullScreen;
			SetMenuCommandIDCheck('vfsc', (m_isFullScreen) ? true : false);
			if (m_isFullScreen == 1)
			{
				short width = m_XWinSize;
				short height = m_YWinSize;
				BeginFullScreen(&m_RestoreState, nil, &width, &height, &m_FullScreenWindow, nil, fullScreenHideCursor);
				fprintf(stderr, "Actual screen dimensions %d, %d\n", width, height);
			}
			else
			{
				EndFullScreen(m_RestoreState, nil);
			}
			break;
		case 'vmar':
            fprintf(stderr, "Maintain Aspect Ratio selected\n");
			m_maintainAspectRatio = 1 - m_maintainAspectRatio;
			SetMenuCommandIDCheck('vmar', (m_maintainAspectRatio) ? true : false);
			break;
			
// Sound volume

        case 'volf':
            fprintf(stderr, "Full Volume selected\n");
			m_MenuIdVolume = IDM_FULLVOLUME;
			TranslateVolume();
            break;

        case 'volh':
            fprintf(stderr, "High Volume selected\n");
			m_MenuIdVolume = IDM_HIGHVOLUME;
			TranslateVolume();
            break;

        case 'volm':
            fprintf(stderr, "Medium Volume selected\n");
			m_MenuIdVolume = IDM_MEDIUMVOLUME;
			TranslateVolume();
            break;

        case 'voll':
            fprintf(stderr, "Low Volume selected\n");
			m_MenuIdVolume = IDM_LOWVOLUME;
			TranslateVolume();
            break;

        case 'sndc':
            fprintf(stderr, "Sound Chip Enabled selected\n");
			SoundChipEnabled = 1 - SoundChipEnabled;
			SetMenuCommandIDCheck('sndc', (SoundChipEnabled) ? true : false);
            break;

        case 'sond':

            fprintf(stderr, "Sound on/off selected\n");

			if (SoundEnabled)
			{
				SetMenuCommandIDCheck('sond', false);
				SoundReset();
				SoundEnabled = 0;
			}
			else
			{
				SoundInit();
				SetMenuCommandIDCheck('sond', true);
				SoundEnabled = 1;
			}
			
            break;

        case 'sped':
			
            fprintf(stderr, "Speech on/off selected\n");
			
			if (SpeechDefault)
			{
				SetMenuCommandIDCheck('sped', false);
				tms5220_stop();
				SpeechDefault = 0;
			}
			else
			{
				tms5220_start();
				if (SpeechEnabled)
				{
					SetMenuCommandIDCheck('sped', true);
					SpeechDefault = 1;
				}
			}
				
			break;

        case 'enet':
			
            fprintf(stderr, "Econet on/off selected\n");
			EconetEnabled = 1 - EconetEnabled;
			if (EconetEnabled)
			{
				ResetBeebSystem(MachineType, TubeEnabled, 0);
				EconetStateChanged = TRUE;
			}
			else
			{
				EconetReset();
			}
			UpdateEconetMenu();
			
			break;
			
        case 'igil':
            fprintf(stderr, "Ignore Illegal Instructions selected\n");
			if (IgnoreIllegalInstructions)
			{
				IgnoreIllegalInstructions = false;
			}
			else
			{
				IgnoreIllegalInstructions = true;
			}
			SetMenuCommandIDCheck('igil', (IgnoreIllegalInstructions) ? true : false);
            break;

        case 'hard':
            fprintf(stderr, "Basic Hardware selected\n");
			BHardware = 1 - BHardware;
			SetMenuCommandIDCheck('hard', (BHardware) ? true : false);
            break;

        case 'docu':
            fprintf(stderr, "Documented Only selected\n");
			OpCodes = 1;
			InitMenu();
            break;

        case 'extr':
            fprintf(stderr, "Common Extras selected\n");
			OpCodes = 2;
			InitMenu();
            break;

        case 'full':
            fprintf(stderr, "Full Set selected\n");
			OpCodes = 3;
			InitMenu();
            break;

			
        case 'ifd0':
        case 'ifd1':
        case 'ifd2':
        case 'ifd3':
			i = cmdID - 'ifd0';
            fprintf(stderr, "Import Files From Disc %d selected\n", i);
			ImportDiscFiles(i);
            break;

		case 'efd0':
        case 'efd1':
        case 'efd2':
        case 'efd3':
			i = cmdID - 'efd0';
            fprintf(stderr, "Export Files To Disc %d selected\n", i);
			ExportDiscFiles(i);
            break;
			
        case 'roma':
        case 'romb':
        case 'romc':
        case 'romd':
        case 'rome':
        case 'romf':
        case 'romg':
        case 'romh':
        case 'romi':
        case 'romj':
        case 'romk':
        case 'roml':
        case 'romm':
        case 'romn':
        case 'romo':
        case 'romp':
			i = cmdID - 'roma';
            fprintf(stderr, "Allow Rom Writed %1x selected\n", i);
			RomWritable[i] = 1 - RomWritable[i];
			SetMenuCommandIDCheck('roma' + i, RomWritable[i] ? true : false);
            break;

// LED's

        case 'ledr':
            fprintf(stderr, "Red LED's selected\n");
			DiscLedColour = 0;
			UpdateLEDMenu();
            break;

        case 'ledg':
            fprintf(stderr, "Green LED's selected\n");
			DiscLedColour = 1;
			UpdateLEDMenu();
            break;

        case 'ledk':
            fprintf(stderr, "Keyboard LED's selected\n");
			LEDs.ShowKB = !LEDs.ShowKB;
			UpdateLEDMenu();
            break;

        case 'ledd':
            fprintf(stderr, "Disc LED's selected\n");
			LEDs.ShowDisc = !LEDs.ShowDisc;
			UpdateLEDMenu();
            break;

        case 'savp':
            fprintf(stderr, "Save Preferences\n");
			SavePreferences();
            break;

// UEF State

        case 'qukl':
            fprintf(stderr, "Quick Load State selected\n");
			char FileName[256];
			sprintf(FileName, "%sbeebstate/quicksave.uef", RomPath);
			LoadUEFState(FileName);
            break;

        case 'quks':
            fprintf(stderr, "Quick Save State selected\n");
			sprintf(FileName, "%sbeebstate/quicksave.uef", RomPath);
			SaveUEFState(FileName);
            break;

        case 'rsts':
            fprintf(stderr, "Load State selected\n");
			RestoreState();
            break;

        case 'savs':
            fprintf(stderr, "Save State selected\n");
			SaveState();
            break;

// Printer

        case 'pfle':
            fprintf(stderr, "Print To File selected\n");

			if (m_MenuIdPrinterPort == IDM_PRINTER_CLIPBOARD)
			{
				SetMenuCommandIDCheck('pclp', false);
				m_printerbufferlen = 0;
			}
				
			if (PrinterFile())
			{
				/* If printer is enabled then need to disable it before changing file */
				if (PrinterEnabled)
					TogglePrinter();
					
				char Title[256];

				MenuRef			menu = nil;
				MenuItemIndex	j;
				OSStatus		err;

				err = GetIndMenuItemWithCommandID(nil, 'pfle', 1, &menu, &j);
				if (!err)
				{
					CFStringRef pTitle;
					sprintf(Title, "File: %s", m_PrinterFileName);
					pTitle = CFStringCreateWithCString (kCFAllocatorDefault, Title, kCFStringEncodingASCII);
					err = SetMenuItemTextWithCFString(menu, j, pTitle);
					if (err) fprintf(stderr, "SetMenuTitle returned err code %ld\n", err);
					CFRelease(pTitle);
				}

				m_MenuIdPrinterPort = IDM_PRINTER_FILE;
				SetMenuCommandIDCheck('pfle', true);
				TranslatePrinterPort();
				TogglePrinter();		// Turn printer back on
			}
            break;

		case 'pclp':
            fprintf(stderr, "Print To Clipboard selected\n");
			
			if (PrinterEnabled)
				TogglePrinter();
				
			if (PrinterFileHandle != NULL)
			{
				fclose(PrinterFileHandle);
				PrinterFileHandle = NULL;
			}
			SetMenuCommandIDCheck('pfle', false);
				
			m_MenuIdPrinterPort = IDM_PRINTER_CLIPBOARD;
			SetMenuCommandIDCheck('pclp', true);
			TranslatePrinterPort();
			TogglePrinter();		// Turn printer back on

			m_printerbufferlen = 0;

			break;
			
        case 'prnt':
            fprintf(stderr, "Printer On/Off selected\n");
			TogglePrinter();
			m_printerbufferlen = 0;
			break;

	
// Keyboard re-mapping

        case 'kmas':
            fprintf(stderr, "Keyboard Map A, S selected\n");
			m_KeyMapAS = !m_KeyMapAS;
			SetMenuCommandIDCheck('kmas', (m_KeyMapAS) ? true : false);
			break;

		case 'copy':
            fprintf(stderr, "Copy selected\n");
			doCopy();
			break;
			
		case 'past':
            fprintf(stderr, "Paste selected\n");
			doPaste();
			break;
			
// AMX Mouse

        case 'amxo':
            fprintf(stderr, "AMX Mouse On/Off selected\n");
			AMXMouseEnabled = !AMXMouseEnabled;
			UpdateAMXMenu();
			AMXCurrentX = AMXTargetX;
			AMXCurrentY = AMXTargetY;
            break;

        case 'amxl':
            fprintf(stderr, "AMX L+R For Middle selected\n");
			AMXLRForMiddle = !AMXLRForMiddle;
			UpdateAMXMenu();
            break;

        case 'amx1':
            fprintf(stderr, "AMX 160x256 selected\n");
			m_MenuIdAMXSize = 1;
			TranslateAMX();
			UpdateAMXMenu();
            break;

        case 'amx3':
            fprintf(stderr, "AMX 320x256 selected\n");
			m_MenuIdAMXSize = 2;
			TranslateAMX();
			UpdateAMXMenu();
            break;

        case 'amx6':
            fprintf(stderr, "AMX 640x256 selected\n");
			m_MenuIdAMXSize = 3;
			TranslateAMX();
			UpdateAMXMenu();
            break;

// If menu option already selected, set to 0, ie disable tick, else select option

        case 'axp5':
            fprintf(stderr, "AMX Adjust +50%c selected\n", '%');
			m_MenuIdAMXAdjust = (m_MenuIdAMXAdjust == 1) ? 0 : 1;
			TranslateAMX();
			UpdateAMXMenu();
            break;

        case 'axp3':
            fprintf(stderr, "AMX Adjust +30%c selected\n", '%');
			m_MenuIdAMXAdjust = (m_MenuIdAMXAdjust == 2) ? 0 : 2;
			TranslateAMX();
			UpdateAMXMenu();
            break;

        case 'axp1':
            fprintf(stderr, "AMX Adjust +10%c selected\n", '%');
			m_MenuIdAMXAdjust = (m_MenuIdAMXAdjust == 3) ? 0 : 3;
			TranslateAMX();
			UpdateAMXMenu();
            break;

        case 'axm1':
            fprintf(stderr, "AMX Adjust -10%c selected\n", '%');
			m_MenuIdAMXAdjust = (m_MenuIdAMXAdjust == 4) ? 0 : 4;
			TranslateAMX();
			UpdateAMXMenu();
            break;

        case 'axm3':
            fprintf(stderr, "AMX Adjust -30%c selected\n", '%');
			m_MenuIdAMXAdjust = (m_MenuIdAMXAdjust == 5) ? 0 : 5;
			TranslateAMX();
			UpdateAMXMenu();
            break;

        case 'axm5':
            fprintf(stderr, "AMX Adjust -50%c selected\n", '%');
			m_MenuIdAMXAdjust = (m_MenuIdAMXAdjust == 6) ? 0 : 6;
			TranslateAMX();
			UpdateAMXMenu();
            break;

		case 'ofwm':
            fprintf(stderr, "Freeze When Minimised selected\n");
			m_FreezeWhenInactive = 1 - m_FreezeWhenInactive;
			SetMenuCommandIDCheck('ofwm', (m_FreezeWhenInactive) ? true : false);
			break;

		case 'msea':
            fprintf(stderr, "Analogue Mouse stick selected\n");
			if (m_MenuIdSticks == 1)
			{
				AtoDInit();
				m_MenuIdSticks = 0;
			}
			else
			{
				AtoDInit();
				m_MenuIdSticks = 1;
			}
			
			SetMenuCommandIDCheck('msea', (m_MenuIdSticks == 1) ? true : false);
			SetMenuCommandIDCheck('msed', (m_MenuIdSticks == 2) ? true : false);
			break;

        case 'msed':
            fprintf(stderr, "Digital Mouse stick selected\n");
			if (m_MenuIdSticks == 2)
			{
				AtoDInit();
				m_MenuIdSticks = 0;
			}
			else
			{
				AtoDInit();
				m_MenuIdSticks = 2;
			}
			
			SetMenuCommandIDCheck('msea', (m_MenuIdSticks == 1) ? true : false);
			SetMenuCommandIDCheck('msed', (m_MenuIdSticks == 2) ? true : false);
            break;

// Tape

        case 'opnt':
            fprintf(stderr, "Load Tape selected\n");
			LoadTape();
            break;
        case 'tpfa':
            fprintf(stderr, "Fast Tape Speed selected\n");
			SetTapeSpeed(750);
			SetTapeSpeedMenu();
			break;
        case 'tpmf':
            fprintf(stderr, "Medium Fast Tape Speed selected\n");
			SetTapeSpeed(1600);
			SetTapeSpeedMenu();
			break;
        case 'tpms':
            fprintf(stderr, "Medium Slow Tape Speed selected\n");
			SetTapeSpeed(3200);
			SetTapeSpeedMenu();
			break;
        case 'tpno':
            fprintf(stderr, "Normal Tape Speed selected\n");
			SetTapeSpeed(5600);
			SetTapeSpeedMenu();
			break;
        case 'tpso':
            fprintf(stderr, "Tape Sound selected\n");
			TapeSoundEnabled = 1 - TapeSoundEnabled;
			SetMenuCommandIDCheck('tpso', TapeSoundEnabled ? true : false);
			break;
        case 'tpcr':
            fprintf(stderr, "Relay Click Sound selected\n");
			RelaySoundEnabled = 1 - RelaySoundEnabled;
			SetMenuCommandIDCheck('tpcr', RelaySoundEnabled ? true : false);
			break;
        case 'ddso':
            fprintf(stderr, "Disc Drive Sound selected\n");
			DiscDriveSoundEnabled = 1 - DiscDriveSoundEnabled;
			SetMenuCommandIDCheck('ddso', DiscDriveSoundEnabled ? true : false);
			break;
        case 'tpre':
            fprintf(stderr, "Rewind Tape selected\n");
			RewindTape();
			break;
        case 'tpul':
            fprintf(stderr, "Unlock Tape selected\n");
			UnlockTape = 1 - UnlockTape;
			SetUnlockTape(UnlockTape);
			SetMenuCommandIDCheck('tpul', UnlockTape ? true : false);
			break;

		case 'tpco':
            fprintf(stderr, "Tape Control selected\n");
			if (TapeControlEnabled)
				TapeControlCloseDialog();
			else
				TapeControlOpenDialog();
			break;
			
		case 'dbgr':
            fprintf(stderr, "Debug selected\n");
			if (DebugEnabled)
			{
				DebugCloseDialog();
				SetMenuCommandIDCheck('dbgr', false);
			}
			else
			{
				DebugOpenDialog();
				SetMenuCommandIDCheck('dbgr', true);
			}
			break;
        
		case 'upbo':
            fprintf(stderr, "User Port Breakout Box\n");
			if (mBreakOutWindow)
			{
				BreakOutCloseDialog();
				SetMenuCommandIDCheck('upbo', false);
			}
				else
				{
					BreakOutOpenDialog();
					SetMenuCommandIDCheck('upbo', true);
				}
				break;
			
		case 'uprm':
            fprintf(stderr, "User Port RTC Module\n");
			RTC_Enabled = 1 - RTC_Enabled;
			SetMenuCommandIDCheck('uprm', RTC_Enabled ? true : false);
			break;
			
		case 'abou':
            fprintf(stderr, "About Menu Selected\n");

			Str255 S1;
			Str255 S2;
			SInt16 r;
			char buff[256];
			
			sprintf(buff, "MacBeebEm Ver %s", Version);
			CopyCStringToPascal(buff, S1);
			sprintf(buff, "Mac Conversion By Jon Welch\n\njon@g7jjf.com\nhttp://www.g7jjf.com\n%s", VersionDate);
			CopyCStringToPascal(buff, S2);
			StandardAlert( kAlertNoteAlert, S1, S2, NULL, &r);
			
			break;
			
		case 'kusr':
            fprintf(stderr, "Define User Keyboard selected\n");
			if (mUKWindow == NULL)
				UserKeyboardOpenDialog();
			else
				UserKeyboardCloseDialog();
			break;

		case 'lukm':
            fprintf(stderr, "Load user key map selected\n");
			LoadUserKeyMap();
			break;
			
		case 'sukm':
            fprintf(stderr, "Save user key map selected\n");
			SaveUserKeyMap();
			break;
			
		case 'udkm':
            fprintf(stderr, "User Defined Keyboard Mapping selected\n");
			m_MenuIdKeyMapping = 1;
			SetMenuCommandIDCheck('dkm ', false);
			SetMenuCommandIDCheck('udkm', true);
			TranslateKeyMapping();
			break;
			
		case 'dkm ':
            fprintf(stderr, "Default Keyboard Mapping selected\n");
			m_MenuIdKeyMapping = 0;
			SetMenuCommandIDCheck('dkm ', true);
			SetMenuCommandIDCheck('udkm', false);
			TranslateKeyMapping();
			break;
			
			// Tape
			
        case 'vidc':
            fprintf(stderr, "Capture Video selected\n");
			SetMenuCommandIDCheck('vidc', true);
			CaptureVideo();
            break;

        case 'vide':
            fprintf(stderr, "End Capture Video selected\n");
			EndCaptureVideo();
            break;

        case 'skp0':
            fprintf(stderr, "Frame Skip 0 selected\n");
			m_skip = 0;
			TranslateCapture();
            break;

        case 'skp1':
            fprintf(stderr, "Frame Skip 1 selected\n");
			m_skip = 1;
			TranslateCapture();
            break;

        case 'skp2':
            fprintf(stderr, "Frame Skip 2 selected\n");
			m_skip = 2;
			TranslateCapture();
            break;

        case 'skp3':
            fprintf(stderr, "Frame Skip 3 selected\n");
			m_skip = 3;
			TranslateCapture();
            break;

        case 'skp4':
            fprintf(stderr, "Frame Skip 4 selected\n");
			m_skip = 4;
			TranslateCapture();
            break;

        case 'skp5':
            fprintf(stderr, "Frame Skip 5 selected\n");
			m_skip = 5;
			TranslateCapture();
            break;
			
        case 'rec1':
            fprintf(stderr, "Capture Resolution @ 768x576 selected\n");
			m_captureresolution = 1;
			TranslateCapture();
            break;

		case 'rec2':
            fprintf(stderr, "Capture Resolution @ Display Resolution selected\n");
			m_captureresolution = 2;
			TranslateCapture();
            break;

		case 'rec3':
            fprintf(stderr, "Capture Resolution @ 1/2 Display Resolution selected\n");
			m_captureresolution = 3;
			TranslateCapture();
            break;

		case 'rec4':
            fprintf(stderr, "Capture Resolution @ 1/4 Display Resolution selected\n");
			m_captureresolution = 4;
			TranslateCapture();
            break;
			
// Motion Blur
			
        case 'mbof':
            fprintf(stderr, "Motion Blur Off selected\n");
			m_Motion_Blur = 0;
			UpdateMotionBlurMenu();
            break;

        case 'mb2f':
            fprintf(stderr, "Motion Blur 2 Frames selected\n");
			m_Motion_Blur = 1;
			UpdateMotionBlurMenu();
            break;
        
		case 'mb4f':
            fprintf(stderr, "Motion Blur 4 Frames selected\n");
			m_Motion_Blur = 2;
			UpdateMotionBlurMenu();
            break;
        
		case 'mb8f':
            fprintf(stderr, "Motion Blur 8 Frames selected\n");
			m_Motion_Blur = 3;
			UpdateMotionBlurMenu();
            break;

		case 'ejd0':
            fprintf(stderr, "Eject Disc 0 selected\n");
			EjectDiscImage(0);
            break;

		case 'ejd1':
            fprintf(stderr, "Eject Disc 1 selected\n");
			EjectDiscImage(1);
            break;
			
		case 'snev':
            fprintf(stderr, "Sound Volume Exponential selected\n");
			SoundExponentialVolume = 1 - SoundExponentialVolume;
			SetMenuCommandIDCheck('snev', (SoundExponentialVolume) ? true : false);
            break;

		case 'txte':
            fprintf(stderr, "TeleText On/Off selected\n");
			TeleTextAdapterEnabled = 1 - TeleTextAdapterEnabled;
			TeleTextInit();
			SetMenuCommandIDCheck('txte', (TeleTextAdapterEnabled) ? true : false);
			if (TeleTextAdapterEnabled == 0)
			{
				TeleTextData = 0;
				TeleTextServer = 0;
				SetMenuCommandIDCheck('txtd', (TeleTextData) ? true : false);
				SetMenuCommandIDCheck('txts', (TeleTextServer) ? true : false);
			}
            break;

		case 'txtd':
            fprintf(stderr, "TeleText Data selected\n");
			if (TeleTextAdapterEnabled == 1)
			{
				TeleTextData = 1 - TeleTextData;
				if (TeleTextData == 1) TeleTextServer = 0;
				SetMenuCommandIDCheck('txtd', (TeleTextData) ? true : false);
				SetMenuCommandIDCheck('txts', (TeleTextServer) ? true : false);
				TeleTextInit();
			}
			break;
			
		case 'txts':
            fprintf(stderr, "TeleText Server selected\n");
			if (TeleTextAdapterEnabled == 1)
			{
				TeleTextServer = 1 - TeleTextServer;
				if (TeleTextServer == 1) TeleTextData = 0;
				SetMenuCommandIDCheck('txtd', (TeleTextData) ? true : false);
				SetMenuCommandIDCheck('txts', (TeleTextServer) ? true : false);
				TeleTextInit();
			}
			break;
			
		case 'hdre':
            fprintf(stderr, "Hard Drive On/Off selected\n");
			HardDriveEnabled = 1 - HardDriveEnabled;
			SCSIReset();
			SASIReset();
			SetMenuCommandIDCheck('hdre', (HardDriveEnabled) ? true : false);
            break;
		
        case 'rs42':
            fprintf(stderr, "RS423 On/Off selected\n");
			
			if (TouchScreenEnabled)
			{
				TouchScreenClose();
				TouchScreenEnabled = false;
			}

			if (EthernetPortEnabled)
			{
				EthernetPortClose();
				EthernetPortEnabled = false;
			}
			
			if (mSerialHandle != -1)
			{
				CloseSerialPort(mSerialHandle);
			}
				
			SerialPortEnabled = 1 - SerialPortEnabled;
			
			SetMenuCommandIDCheck('rs42', (SerialPortEnabled) ? true : false);
			SetMenuCommandIDCheck('sdts', (TouchScreenEnabled) ? true : false);
			SetMenuCommandIDCheck('sdep', (EthernetPortEnabled) ? true : false);
			SetMenuCommandIDCheck('sdsp', (mSerialHandle != -1) ? true : false);
			
			break;
			
        case 'sdts':
            fprintf(stderr, "Touch Screen selected\n");
			
			if (mSerialHandle != -1)
			{
				CloseSerialPort(mSerialHandle);
			}

			if (TouchScreenEnabled)
			{
				TouchScreenClose();
			}

			if (EthernetPortEnabled)
			{
				EthernetPortClose();
			}
						
			TouchScreenEnabled = 1 - TouchScreenEnabled;
			SetMenuCommandIDCheck('sdts', (TouchScreenEnabled) ? true : false);
			
			if (TouchScreenEnabled)
			{
				
				// Also switch on analogue mousestick as touch screen uses mousestick position
				
				m_MenuIdSticks = 1;
				AtoDInit();
				SetMenuCommandIDCheck('msea', (m_MenuIdSticks == 1) ? true : false);
				SetMenuCommandIDCheck('msed', (m_MenuIdSticks == 2) ? true : false);
				
				SerialPortEnabled = true;
				SetMenuCommandIDCheck('rs42', (SerialPortEnabled) ? true : false);
				SetMenuCommandIDCheck('sdsp', false);
				TouchScreenOpen();
			}
			break;

		case 'sdep':
            fprintf(stderr, "Remote Ethernet Port ... selected\n");
			
			if (mSerialHandle != -1)
			{
				CloseSerialPort(mSerialHandle);
			}
			
			if (TouchScreenEnabled)
			{
				TouchScreenClose();
				TouchScreenEnabled = false;
				SetMenuCommandIDCheck('sdts', false);
			}

			if (mEthernetPortWindow == NULL)
			{
				EthernetPortOpenDialog();
				EthernetPortEnabled = true;
				SerialPortEnabled = true;
				SetMenuCommandIDCheck('rs42', true);
				SetMenuCommandIDCheck('sdep', true);
			}
			else
			{
				if (mEthernetHandle > 0)
				{
					EthernetPortClose();
				}
				EthernetPortCloseDialog();
				EthernetPortEnabled = false;
				SerialPortEnabled = false;
				SetMenuCommandIDCheck('rs42', false);
				SetMenuCommandIDCheck('sdep', false);
			}
			break;
			
		case 'sdsp':
            fprintf(stderr, "Serial Port... selected\n");

			if (TouchScreenEnabled)
			{
				TouchScreenClose();
				TouchScreenEnabled = false;
				SetMenuCommandIDCheck('sdts', (TouchScreenEnabled) ? true : false);
			}

			if (EthernetPortEnabled)
			{
				EthernetPortClose();
				EthernetPortEnabled = false;
				SetMenuCommandIDCheck('sdep', (EthernetPortEnabled) ? true : false);
			}
			
			if (mSerialPortWindow == NULL)
			{
				SerialPortOpenDialog();
				SerialPortEnabled = true;
				SetMenuCommandIDCheck('rs42', (SerialPortEnabled) ? true : false);
				SetMenuCommandIDCheck('sdsp', true);
			}
			else
			{
				if (mSerialHandle != -1)
				{
					CloseSerialPort(mSerialHandle);
				}
				SerialPortCloseDialog();
				SetMenuCommandIDCheck('sdsp', false);
			}
			break;

		case 'page':
            fprintf(stderr, "Page Setup... selected\n");
			DoPageSetup();
			break;

		case 'prns':
            fprintf(stderr, "Print... selected\n");
			m_PrintToPDF = true;
			break;

		case 'cpyc':
            fprintf(stderr, "Copy To Clipboard selected\n");
			m_CopyToClip = true;
			break;
			
		case 'invb':
            fprintf(stderr, "Invert Background selected\n");
			m_Invert = 1 - m_Invert;
			SetMenuCommandIDCheck('invb', (m_Invert) ? true : false);
		
		case 'swtd':
            fprintf(stderr, "Save Window To Disc ... selected\n");
			m_PrintScreen = true;
			break;

		case 'mbcn':
            fprintf(stderr, "Native 8271 Controller selected\n");
			NativeFDC = true;
			FDCType = 0;
			TranslateFDC();
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			break;

		case 'mbca':
            fprintf(stderr, "Acorn 1770 Controller selected\n");
			NativeFDC = false;
			FDCType = 0;
			TranslateFDC();
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			break;

		case 'mbco':
            fprintf(stderr, "OPUS 1770 Controller selected\n");
			NativeFDC = false;
			FDCType = 1;
			TranslateFDC();
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			break;

		case 'mbcw':
            fprintf(stderr, "Watford 1770 Controller selected\n");
			NativeFDC = false;
			FDCType = 2;
			TranslateFDC();
			ResetBeebSystem(MachineType, TubeEnabled, 0);
			break;
			
			default:
            err = eventNotHandledErr;
            break;
    }
    
	return err;
}

void BeebWin::NewDiscImage(int Drive)

{
char path[256];  
OSErr err = noErr;

	*path = 0;
	err = SaveFile(path, nil);

// For some reason, Return key sticks down if replace an existing file

	KeyUp(36);

	if (err != noErr) return;
	if (*path == 0) return;

	if (strstr(path, "ssd")) CreateDiscImage(path, Drive, 1, 80);
	if (strstr(path, "dsd")) CreateDiscImage(path, Drive, 2, 80);
	if (strstr(path, "adf")) CreateADFSImage(path, Drive, 80);
	if (strstr(path, "adl")) CreateADFSImage(path, Drive, 160);
			
	LoadDisc(Drive, path);
	
//	if (m_WriteProtectDisc[Drive])
//		ToggleWriteProtect(Drive);
//	DWriteable[Drive] = 1;
//	DiscLoaded[Drive] = true;
//	strcpy(CDiscName[1], path);
}


void BeebWin::NewTapeImage(char *FileName)

{
	char path[256];  
	OSErr err = noErr;
	
	*FileName = 0;
	*path = 0;
	err = SaveFile(path, nil);
	
	// For some reason, Return key sticks down if replace an existing file
	
	KeyUp(36);
	
	if (err != noErr) return;
	
	if ( ! ((strstr(path, ".UEF")) ||(strstr(path, ".UEF"))) )
		strcat(path, ".uef");

	strcpy(FileName, path);
}

void BeebWin::SaveState()
{
char path[256];  
OSErr err = noErr;

	*path = 0;
	err = SaveFile(path, nil);

// For some reason, Return key sticks down if replace an existing file

	KeyUp(36);

	if (err != noErr) return;
	if (*path == 0) return;

	if ( ! ((strstr(path, ".UEF")) ||(strstr(path, ".UEF"))) )
		strcat(path, ".uef");
	
	SaveUEFState(path);
}

void BeebWin::RestoreState()
{
char path[256];  
OSErr err = noErr;

	err = GetOneFileWithPreview(path, UEFFilterProc);
	if (err) return;
	LoadUEFState(path);
}

bool BeebWin::PrinterFile()
{
char path[256];  
OSErr err = noErr;

	*path = 0;
	err = SaveFile(path, nil);

// For some reason, Return key sticks down if replace an existing file

	KeyUp(36);

	if (err != noErr) return false;
	if (*path == 0) return false;

	strcpy(m_PrinterFileName, path);
	return true;
}

void BeebWin::TogglePrinter()
{
	if (PrinterEnabled) {
		PrinterDisable();
	}
	else
	{
		if (m_MenuIdPrinterPort == IDM_PRINTER_FILE)
		{
			if (strlen(m_PrinterFileName) == 0)
				PrinterFile();
			else
				PrinterEnable(m_PrinterFileName);
		}
		else
		{
			PrinterEnable(m_PrinterDevice);
		}
	}

	SetMenuCommandIDCheck('prnt', (PrinterEnabled) ? true : false);

}

void BeebWin::TranslatePrinterPort()
{
	switch (m_MenuIdPrinterPort)
	{
	case IDM_PRINTER_FILE :
		strcpy(m_PrinterDevice, m_PrinterFileName);
		break;
	case IDM_PRINTER_CLIPBOARD :
		strcpy(m_PrinterDevice, "CLIPBOARD");
		break;
	}
}


void SaveEmuUEF(FILE *SUEF) {
	char EmuName[16];
	fput16(0x046C,SUEF);
	fput32(16,SUEF);
	// BeebEm Title Block
	strcpy(EmuName,"BeebEm");
	EmuName[14]=3;
	EmuName[15]=0; // Version, 3.0
	fwrite(EmuName,16,1,SUEF);
	//
	fput16(0x046a,SUEF);
	fput32(16,SUEF);
	// Emulator Specifics
	// Note about this block: It should only be handled by beebem from uefstate.cpp if
	// the UEF has been determined to be from BeebEm (Block 046C)
	fputc(MachineType,SUEF);
	fputc((NativeFDC)?0:1,SUEF);
	fputc(TubeEnabled,SUEF);
	fputc(0,SUEF); // Monitor type, reserved
	fputc(0,SUEF); // Speed Setting, reserved
	fput32(0,SUEF);
	fput32(0,SUEF);
	fput16(0,SUEF);
	fputc(0,SUEF);
}

void LoadEmuUEF(FILE *SUEF, int Version) {
	MachineType=fgetc(SUEF);
	if (Version <= 8 && MachineType == 1)
		MachineType = 3;
	NativeFDC=(fgetc(SUEF)==0)?TRUE:FALSE;
	TubeEnabled=fgetc(SUEF);
	mainWin->ResetBeebSystem(MachineType,TubeEnabled,1);
	mainWin->UpdateModelType();
}

void BeebWin::LoadUserKeyMap ()
{
	OSErr err = noErr;
	char path[256];
	
	err = GetOneFileWithPreview(path, KeyboardFilterProc);
	if (err != noErr) return;
	
	LoadUserKeyboard(path);
}

void BeebWin::SaveUserKeyMap ()
{
	OSErr err = noErr;
	char path[256];
	
	err = SaveFile(path, nil);
	if (err != noErr) return;
	
	if ( ! ((strstr(path, ".KMAP")) ||(strstr(path, ".kmap"))) )
		strcat(path, ".kmap");
	
	SaveUserKeyboard (path);
}

void BeebWin::SetAMXPosition(unsigned int x, unsigned int y)
{
	if ( (x < 0) || (x >= mainWin->m_XWinSize) ) return;
	if ( (y < 0) || (y >= mainWin->m_YWinSize) ) return;
	
	if (AMXMouseEnabled)
	{

		AMXTargetX = (int) (x * (m_AMXXSize * (100.0 + m_AMXAdjust) / 100.0 / m_XWinSize));
		AMXTargetY = (int) (y * (m_AMXYSize * (100.0 + m_AMXAdjust) / 100.0 / m_YWinSize));
		
//		WriteLog("X = %d, Y = %d, B = %d\n", AMXTargetX, AMXTargetY, AMXButtons);
		AMXMouseMovement();
	}
}

void BeebWin::SetMousestickButton(int button)
{
	if (m_MenuIdSticks)
	{
		JoystickButton = button;
	}
}

void BeebWin::ScaleMousestick(unsigned int x, unsigned int y)
{
static int lastx = 32768;
static int lasty = 32768;
int dx, dy;

	if ( (x < 0) || (x >= mainWin->m_XWinSize) ) return;
	if ( (y < 0) || (y >= mainWin->m_YWinSize) ) return;
	
	if (m_MenuIdSticks == 1)		// Analogue Mousestick
	{
		JoystickX = (m_XWinSize - x) * 65535 / m_XWinSize;
		JoystickY = (m_YWinSize - y) * 65535 / m_YWinSize;
		return;
	}

	if (m_MenuIdSticks == 2)		// Digital Mousestick
	{
		dx = x - lastx;
		dy = y - lasty;
		
		if (dx > 4) JoystickX = 0;
		if (dx < -4) JoystickX = 65535;

		if (dy > 4) JoystickY = 0;
		if (dy < -4) JoystickY = 65535;

		lastx = x;
		lasty = y;
	}

}

void BeebWin::TranslateAMX(void)
{
	switch (m_MenuIdAMXSize)
	{
	case 1 :
		m_AMXXSize = 160;
		m_AMXYSize = 256;
		break;
	default:
	case 2 :
		m_AMXXSize = 320;
		m_AMXYSize = 256;
		break;
	case 3 :
		m_AMXXSize = 640;
		m_AMXYSize = 256;
		break;
	}

	switch (m_MenuIdAMXAdjust)
	{
	case 0 :
		m_AMXAdjust = 0;
		break;
	case 1 :
		m_AMXAdjust = 50;
		break;
	default:
	case 2 :
		m_AMXAdjust = 30;
		break;
	case 3 :
		m_AMXAdjust = 10;
		break;
	case 4 :
		m_AMXAdjust = -10;
		break;
	case 5 :
		m_AMXAdjust = -30;
		break;
	case 6 :
		m_AMXAdjust = -50;
		break;
	}
}

void BeebWin::CaptureVideo()
{
	FSSpec fileSpec;
	OSErr err = noErr;
	char path[256];  
	
	if (m_pMovie)
		EndCaptureVideo();

	*path = 0;
	err = SaveFileMov(path, &fileSpec);
	
	// For some reason, Return key sticks down if replace an existing file
	
	KeyUp(36);
	
	if (err != noErr) return;
	
	StartRecordingVideo(path, &fileSpec);
}

void MacQTOpenVideoComponent(ComponentInstance *rci)
{	
	OSStatus			err;
	ComponentInstance	ci;
	
	ci = OpenDefaultComponent(StandardCompressionType, StandardCompressionSubType);
	
	CFDataRef	data;
	
	data = (CFDataRef) CFPreferencesCopyAppValue(CFSTR("QTVideoSetting"), kCFPreferencesCurrentApplication);
	if (data)
	{
		CFIndex	len;
		Handle	hdl;
		
		len = CFDataGetLength(data);
		hdl = NewHandleClear((Size) len);
		if (MemError() == noErr)
		{	
			HLock(hdl);
			CFDataGetBytes(data, CFRangeMake(0, len), (unsigned char *) *hdl);
			err = SCSetInfo(ci, scSettingsStateType, &hdl);
			
			DisposeHandle(hdl);
		}
		
		CFRelease(data);
	}
	
	*rci = ci;
}

void MacQTCloseVideoComponent(ComponentInstance ci)
{
	OSStatus	err;
	
	err = CloseComponent(ci);
}

void BeebWin::StartRecordingVideo(char *path, FSSpec *fs)
{
	OSErr err = noErr;
	FSSpec fileSpec;
	Str255 FileName;
	
	if (m_pMovie)
		EndCaptureVideo();
	
	if (fs == nil)
	{
		CopyCStringToPascal(path, FileName);
		err = FSMakeFSSpec(0, 0, FileName, &fileSpec);
	}
	else
	{
		memcpy(&fileSpec, fs, sizeof(FSSpec));
	}
	
	MacQTOpenVideoComponent(&m_ci);
	
	long	flag;
	
	flag = scListEveryCodec | scAllowZeroKeyFrameRate | scDisableFrameRateItem;
	err = SCSetInfo(m_ci, scPreferenceFlagsType, &flag);
	
	CGrafPtr mWin;
	PixMapHandle	pmh;
	
	mWin = GetWindowPort(mWindow);
	SetPortWindowPort(mWindow);
	
	LockPortBits(mWin);
	
	pmh = GetPortPixMap(mWin);
	
	LockPixels(pmh);

	err = SCSetTestImagePixMap(m_ci, pmh, nil, scPreferScaling);
	
	err = SCRequestSequenceSettings(m_ci);

	UnlockPixels(pmh);
	UnlockPortBits(mWin);

	if (err == noErr)
	{
		CFDataRef	data;
		Handle		hdl;
		
		err = SCGetInfo(m_ci, scSettingsStateType, &hdl);
		if (err == noErr)
		{
			HLock(hdl);
			data = CFDataCreate(kCFAllocatorDefault, (unsigned char *) *hdl, GetHandleSize(hdl));
			if (data)
			{
				CFPreferencesSetAppValue(CFSTR("QTVideoSetting"), data, kCFPreferencesCurrentApplication);
				CFRelease(data);
			}
			
			DisposeHandle(hdl);
		}
	}
	
	MacQTCloseVideoComponent(m_ci);
	
	if (err != noErr) return;
	
	MacQTOpenVideoComponent(&m_ci);
	
	SCTemporalSettings	ts;
	
	err = SCGetInfo(m_ci, scTemporalSettingsType, &ts);
	ts.frameRate = FixRatio( (long) m_FramesPerSecond, 1);
	if (ts.keyFrameRate < 1)
		ts.keyFrameRate = (long) m_FramesPerSecond;
	m_keyFrame  = m_keyFrameCount  = ts.keyFrameRate;
	m_frameSkip = m_frameSkipCount = m_skip;					// Miss out every other frame
	err = SCSetInfo(m_ci, scTemporalSettingsType, &ts);

	err = EnterMovies();
	
	err = CreateMovieFile(&fileSpec, FOUR_CHAR_CODE('TVOD'), smCurrentScript, createMovieFileDeleteCurFile | createMovieFileDontCreateResFile,
						  &m_resRefNum, &m_pMovie);
	
	if (err != noErr)
	{
		fprintf(stderr, "CreateMovieFailed with return code %d\n", err);
	}
	
	m_trackFrame.left = 0;
	m_trackFrame.top = 22;
	m_trackFrame.right = m_XWinSize;
	m_trackFrame.bottom = m_YWinSize + 22;
	
	m_firstFrame = true;
	
	switch (m_captureresolution)
	{
		case 1 :
			m_pTrack = NewMovieTrack(m_pMovie, FixRatio(768, 1), FixRatio(576, 1), 0);
			break;
		case 2 :
			m_pTrack = NewMovieTrack(m_pMovie, FixRatio(m_XWinSize, 1), FixRatio(m_YWinSize, 1), 0);
			break;
		case 3 :
			m_pTrack = NewMovieTrack(m_pMovie, FixRatio(m_XWinSize / 2, 1), FixRatio(m_YWinSize / 2, 1), 0);
			break;
		case 4 :
			m_pTrack = NewMovieTrack(m_pMovie, FixRatio(m_XWinSize / 4, 1), FixRatio(m_YWinSize / 4, 1), 0);
			break;
	}
	
	m_pMedia = NewTrackMedia(m_pTrack, VideoMediaType, 44100, nil, 0);
	
	// sound
	
	m_soundDesc = (SoundDescriptionHandle) NewHandleClear(sizeof(SoundDescription));
	
	(**m_soundDesc).descSize    = sizeof(SoundDescription);
	(**m_soundDesc).dataFormat  = k8BitOffsetBinaryFormat;
	(**m_soundDesc).numChannels = 1;
	(**m_soundDesc).sampleSize  = 8;
	(**m_soundDesc).sampleRate  = (UnsignedFixed) FixRatio(44100, 1);
	
	m_soundBuffer = NewHandle(32768);
	MoveHHi(m_soundBuffer);
	HLock(m_soundBuffer);
	m_soundBufferPtr = *m_soundBuffer;
	m_soundBufferLen = 0;

	m_sTrack = NewMovieTrack(m_pMovie, 0, 0, kFullVolume);
	m_sMedia = NewTrackMedia(m_sTrack, SoundMediaType, 44100, nil, 0);
	
	err = BeginMediaEdits(m_pMedia);
	err = BeginMediaEdits(m_sMedia);
	
}

void BeebWin::EndCaptureVideo()

{
OSErr err;
short resId =movieInDataForkResID;
	
	if (m_pMovie == NULL) return;
	
	SetMenuCommandIDCheck('vidc', false);

	err = SCCompressSequenceEnd(m_ci);
	if (err != noErr) fprintf(stderr, "SCCompressSequenceEnd error %d\n", err);
		
	if (m_pMedia) EndMediaEdits(m_pMedia);
	if (m_pTrack) InsertMediaIntoTrack(m_pTrack, 0, 0, GetMediaDuration(m_pMedia), fixed1);

	if (m_sMedia) EndMediaEdits(m_sMedia);
	if (m_sTrack) InsertMediaIntoTrack(m_sTrack, 0, 0, GetMediaDuration(m_sMedia), fixed1);

	if (m_pMovie) AddMovieResource(m_pMovie, m_resRefNum, &resId, NULL);
	
	if (m_resRefNum != 0) CloseMovieFile(m_resRefNum);
	
	MacQTCloseVideoComponent(m_ci);
	
	if (m_soundDesc)
	{
		DisposeHandle( (Handle) m_soundDesc);
		m_soundDesc = NULL;
	}
	
	if (m_soundBuffer)
	{
		DisposeHandle( (Handle) m_soundBuffer);
		m_soundBuffer = NULL;
	}

	if (m_pMovie)
	{
		DisposeMovie(m_pMovie);
		m_pMovie = NULL;
	}
	
	ExitMovies();
	
	m_pMovie = NULL;
	
}

void BeebWin::doCopy()
{
	
	if (PrinterEnabled)
		TogglePrinter();

	if (PrinterFileHandle != NULL)
	{
		fclose(PrinterFileHandle);
		PrinterFileHandle = NULL;
	}
	SetMenuCommandIDCheck('pfle', false);
		
	m_MenuIdPrinterPort = IDM_PRINTER_CLIPBOARD;
	SetMenuCommandIDCheck('pclp', true);
	TranslatePrinterPort();
	TogglePrinter();		// Turn printer back on
		
	m_printerbufferlen = 0;
		
	m_clipboard[0] = 2;
	m_clipboard[1] = 'L';
	m_clipboard[2] = '.';
	m_clipboard[3] = 13;
	m_clipboard[4] = 3;
	m_clipboardlen = 5;
	m_clipboardptr = 0;
	m_printerbufferlen = 0;
	SetupClipboard();
}

void BeebWin::doPaste()
{
PasteboardRef outPasteboard;
OSStatus err;
ItemCount itemCount;

	err = PasteboardCreate(kPasteboardClipboard, &outPasteboard);

	// Count the number of items on the pasteboard so we can iterate through them.
	err = PasteboardGetItemCount( outPasteboard, &itemCount );

	for( UInt32 itemIndex = 1; itemIndex <= itemCount; itemIndex++ )
	{
		PasteboardItemID  itemID;
		CFArrayRef      flavorTypeArray;
		CFIndex        flavorCount;
				
		// Every item is identified by a unique value.
		err = PasteboardGetItemIdentifier( outPasteboard, itemIndex, &itemID );
				
		// The item's flavor types are retreived as an array which we are responsible for
		// releaseing later. It's important to take into account all flavors, their flags
		// and the context the data will be used when deciding which flavor ought to be used.
		// The flavor type array is a CFType and we'll need to call CFRelease on it later.
		err = PasteboardCopyItemFlavors( outPasteboard, itemID, &flavorTypeArray );
				
		// Count the number of flavors in the item so we can iterate through them.
		flavorCount = CFArrayGetCount( flavorTypeArray );
				
		for( CFIndex flavorIndex = 0; flavorIndex < flavorCount; flavorIndex++ )
		{
			CFStringRef flavorType;
			CFDataRef   flavorData;
			CFIndex     flavorDataSize;
			char        flavorTypeStr[128];
					
			// grab the flavor name so we can extract it's flags and data
			flavorType = (CFStringRef)CFArrayGetValueAtIndex( flavorTypeArray, flavorIndex );
					
			// Having looked at the item's flavors and their flags we've settled on the data
			// we want to reteive.  Because we're copying the flavor data we'll need to
			// dispose of it via CFRelease when we no longer need it.
			err = PasteboardCopyItemFlavorData( outPasteboard, itemID, flavorType, &flavorData );
					
			flavorDataSize = CFDataGetLength( flavorData );
					
			// Now that we have the flavor, flags, and data we need to format it nicely for the text view.
			CFStringGetCString( flavorType, flavorTypeStr, 128, kCFStringEncodingMacRoman );
					
			if (strcmp(flavorTypeStr, "com.apple.traditional-mac-plain-text") == 0)
			{
				for( short dataIndex = 0; dataIndex <= flavorDataSize; dataIndex++ )
				{
					char byte = *(CFDataGetBytePtr( flavorData ) + dataIndex);
					fprintf(stderr, "Clipboard %d = %d\n", dataIndex, byte);
					m_clipboard[dataIndex] = byte;
				}
						
				m_clipboardlen = flavorDataSize;
				m_clipboardptr = 0;
				SetupClipboard();
			}
					
		}
				
		CFRelease(flavorTypeArray);
	}

	CFRelease(outPasteboard);

}

//--------------------------------------------------------------------------------------------------
/////////////////////////////// Support for Copy/Paste of PDF Data /////////////////////////////////
//--------------------------------------------------------------------------------------------------

// To create PDF data for the Pasteboard, we need to set up a CFDataConsumer that collects data in a CFMutableDataRef.
// Here are the two required callbacks:

static size_t MyCFDataPutBytes(void* info, const void* buffer, size_t count)
{
	CFDataAppendBytes((CFMutableDataRef)info, (const UInt8 *) buffer, count);
	return count;
}

static void MyCFDataRelease(void* info)
{
	CFRelease((CFMutableDataRef)info);
}

void MyDrawIntoPDFPage(CGContextRef pdfContext, PMRect pageRect, int starty, int nlines)
{
	CGrafPtr mWin;
	register int i, j;
	char *p;
	
    Rect destR;
    Rect srcR;
	
	mWin = GetWindowPort(mainWin->mWindow);
	SetPortWindowPort(mainWin->mWindow);
	GetPortBounds(mWin, &destR);
	
	srcR.left = 0;
	srcR.right = ActualScreenWidth;
	
	if (TeletextEnabled)
	{
		srcR.top = 0;
		srcR.bottom = 512;
	}
	else
	{
		srcR.top = starty;
		srcR.bottom = starty + nlines;
	}
	
	long *pPtr32;
	short *pPtr16;
	
	CGContextRef myBitmapContext;
	
	PixMapHandle	pmh;
	int				bpp;
	char			col;
	
	LockPortBits(mWin);
	pmh = GetPortPixMap(mWin);
	LockPixels(pmh);
	bpp = GetPixDepth(pmh);
	
	UnlockPixels(pmh);
	UnlockPortBits(mWin);
	
	myBitmapContext = MyCreateBitmapContext(srcR.right - srcR.left, srcR.bottom - srcR.top, bpp);
	
	pPtr32 = (long *) CGBitmapContextGetData (myBitmapContext);
	pPtr16 = (short *) CGBitmapContextGetData (myBitmapContext);
	
	p = mainWin->m_screen;
	
	for (j = srcR.top; j < srcR.bottom; ++j)
	{
		p = mainWin->m_screen + j * 800 + srcR.left;
		
		for (i = srcR.left; i < srcR.right; ++i)
		{
			col = *p++;

			switch (bpp)
			{
				case 32 :
					if (mainWin->m_Invert)
					{
						if (col == 0) *pPtr32++ = 0xffffffff;
						else if (col == 7) *pPtr32++ = ( (mainWin->palette_type == mainWin->AMBER) || (mainWin->palette_type == mainWin->GREEN) ) ? mainWin->m_RGB32[7] : mainWin->m_RGB32[0];
						else *pPtr32++ = mainWin->m_RGB32[col];
					}
					else *pPtr32++ = mainWin->m_RGB32[col];
					break;
				case 16 :
					if (mainWin->m_Invert)
					{
						if (col == 0) *pPtr16++ = 0x7fff;
						else if (col == 7) *pPtr16++ = ( (mainWin->palette_type == mainWin->AMBER) || (mainWin->palette_type == mainWin->GREEN) ) ? mainWin->m_RGB16[7] : mainWin->m_RGB16[0];
						else *pPtr16++ = mainWin->m_RGB16[col];
					}
					else *pPtr16++ = mainWin->m_RGB16[col];
					break;
			}
		}
	}
	
	CGImageRef myImage;
	
	//	CGColorSpaceRef genericColorSpace = GetGenericRGBColorSpace();
	
	CGRect  docRect = CGRectMake (pageRect.left, pageRect.top, pageRect.right, pageRect.bottom);
	
//	CGContextBeginPage(pdfContext, &docRect);
	
	// ensure that we are drawing in the correct color space, a calibrated color space

	CGColorSpaceRef colorSpace;
	colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	
	CGContextSetFillColorSpace(pdfContext, colorSpace); 
	CGContextSetStrokeColorSpace(pdfContext, colorSpace); 
	
	CGColorSpaceRelease(colorSpace);

	myImage = CGBitmapContextCreateImage(myBitmapContext);
	
	docRect.origin.x = pageRect.right / 2 - (destR.right - destR.left) / 2;
	docRect.origin.y = pageRect.bottom / 2 - (destR.bottom - destR.top) / 2;

	docRect.size.width = (destR.right - destR.left);
	docRect.size.height = (destR.bottom - destR.top);
	
	CGContextDrawImage(pdfContext, docRect, myImage);

//	CGContextEndPage(pdfContext);
	
	CGContextRelease(myBitmapContext);
	
	CGImageRelease(myImage);

}

void CopyToClipBoardAsPDF(int starty, int nlines)
{
	//	fprintf(stderr, "Print %d (%c) to clipboard\n", Value, Value);
	
	PasteboardRef outPasteboard;
	OSStatus err;
	
	CGRect  docRect = CGRectMake (0, 0, mainWin->m_XWinSize, mainWin->m_YWinSize);
	
	CFDataRef       pdfData = CFDataCreateMutable (kCFAllocatorDefault, 0);
	CGContextRef            pdfContext;
	CGDataConsumerRef       consumer;
	CGDataConsumerCallbacks cfDataCallbacks = {MyCFDataPutBytes, MyCFDataRelease };
	
	err = PasteboardCreate(kPasteboardClipboard, &outPasteboard);
	
	err = PasteboardClear( outPasteboard );
	
	consumer = CGDataConsumerCreate ((void*)pdfData, &cfDataCallbacks);// 2
	
	pdfContext = CGPDFContextCreate (consumer, &docRect, NULL);// 3
	
    PMRect	pageRect;
	pageRect.top = 0;
	pageRect.left = 0;
	pageRect.bottom = mainWin->m_YWinSize;
	pageRect.right = mainWin->m_XWinSize;
	
	CGContextBeginPage(pdfContext, &docRect);
	
	MyDrawIntoPDFPage (pdfContext, pageRect, starty, nlines);
	
	CGContextEndPage(pdfContext);
	
	CGContextRelease (pdfContext);
	
	PasteboardPutItemFlavor( outPasteboard, (PasteboardItemID)1,
							kUTTypePDF, pdfData, kPasteboardFlavorNoFlags );
	
	CGDataConsumerRelease (consumer);
	
	CFRelease(outPasteboard);
	
}

/****************************************************************************/
/* Disc Import / Export */

static DFS_DISC_CATALOGUE dfsCat;
static int filesSelected[DFS_MAX_CAT_SIZE];
static int numSelected;
static char szExportFolder[MAX_PATH];

// File export

/*
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
		case BFFM_INITIALIZED:
			if (szExportFolder[0])
			{
				SendMessage(hwnd, BFFM_SETEXPANDED, TRUE, (LPARAM)szExportFolder);
				SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)szExportFolder);
			}
			break;
	}
	return 0;
}
*/

/*
BOOL CALLBACK DiscExportDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char str[100];
	HWND hwndList;
	char szDisplayName[MAX_PATH];
	int i, j;
	
	hwndList = GetDlgItem(hwndDlg, IDC_EXPORTFILELIST);
	
	switch (message)
	{
		case WM_INITDIALOG:
			SendMessage(hwndList, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), (LPARAM)MAKELPARAM(FALSE,0));
			
			for (i = 0; i < dfsCat.numFiles; ++i)
			{
				sprintf(str, "%c.%-7s %06X %06X %06X",
						dfsCat.fileAttrs[i].directory,
						dfsCat.fileAttrs[i].filename,
						dfsCat.fileAttrs[i].loadAddr & 0xffffff,
						dfsCat.fileAttrs[i].execAddr & 0xffffff,
						dfsCat.fileAttrs[i].length);
				j = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)str);
				// List is sorted so store catalogue index in list's item data
				SendMessage(hwndList, LB_SETITEMDATA, j, (LPARAM)i);
			}
			return TRUE;
			
		case WM_COMMAND:
			switch (LOWORD(wParam))
		{
			case IDOK:
				numSelected = (int)SendMessage(
											   hwndList, LB_GETSELITEMS, (WPARAM)DFS_MAX_CAT_SIZE, (LPARAM)filesSelected);
				if (numSelected)
				{
					// Convert list indices to catalogue indices
					for (i = 0; i < numSelected; ++i)
					{
						filesSelected[i] = (int)SendMessage(hwndList, LB_GETITEMDATA, filesSelected[i], 0);
					}
					
					// Get folder to export to
					BROWSEINFO bi;
					memset(&bi, 0, sizeof(bi));
					bi.hwndOwner = hwndDlg; // m_hWnd;
					bi.pszDisplayName = szDisplayName;
					bi.lpszTitle = "Select folder for exported files:";
					bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE;
					bi.lpfn = BrowseCallbackProc;
					LPITEMIDLIST idList = SHBrowseForFolder(&bi);
					if (idList == NULL)
					{
						wParam = IDCANCEL;
					}
					else if (SHGetPathFromIDList(idList, szExportFolder) == FALSE)
					{
						MessageBox(hwndDlg, "Invalid folder selected", WindowTitle, MB_OK|MB_ICONWARNING);
						wParam = IDCANCEL;
					}
					if (idList != NULL)
						CoTaskMemFree(idList);
				}
				
				EndDialog(hwndDlg, wParam);
				return TRUE;
				
			case IDCANCEL:
				EndDialog(hwndDlg, wParam);
				return TRUE;
		}
	}
	return FALSE;
}
*/
void BeebWin::ExportDiscFiles(int menuId)
{
/*	
	bool success = true;
	int drive;
	int type;
	char szDiscFile[MAX_PATH];
	int heads;
	int side;
	char szErrStr[500];
	int i, n;
	
	if (menuId == 0 || menuId == 2)
		drive = 0;
	else
		drive = 1;
	
	if (MachineType != 3 && NativeFDC)
	{
		// 8271 controller
		Get8271DiscInfo(drive, szDiscFile, &heads);
	}
	else
	{
		// 1770 controller
		Get1770DiscInfo(drive, &type, szDiscFile);
		if (type == 0)
			heads = 1;
		else if (type == 1)
			heads = 2;
		else
		{
			// ADFS - not currently supported
			MessageBox(m_hWnd, "Export from ADFS disc not supported", WindowTitle, MB_OK|MB_ICONWARNING);
			return;
		}
	}
	
	// Check for no disk loaded
	if (szDiscFile[0] == 0 || heads == 1 && (menuId == 2 || menuId == 3))
	{
		sprintf(szErrStr, "No disc loaded in drive %d", menuId);
		MessageBox(m_hWnd, szErrStr, WindowTitle, MB_OK|MB_ICONWARNING);
		return;
	}
	
	// Read the catalogue
	if (menuId == 0 || menuId == 1)
		side = 0;
	else
		side = 1;
	
	success = dfs_get_catalogue(szDiscFile, heads, side, &dfsCat);
	if (!success)
	{
		sprintf(szErrStr, "Failed to read catalogue from disc:\n  %s", szDiscFile);
		MessageBox(m_hWnd, szErrStr, WindowTitle, MB_OK|MB_ICONERROR);
		return;
	}
	
	// Show export dialog
	if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DISCEXPORT), m_hWnd, (DLGPROC)DiscExportDlgProc) != IDOK ||
		numSelected == 0)
	{
		return;
	}
	
	// Export the files
	n = 0;
	for (i = 0; i < numSelected; ++i)
	{
		success = dfs_export_file(szDiscFile, heads, side, &dfsCat,
								  filesSelected[i], szExportFolder, szErrStr);
		if (success)
		{
			n++;
		}
		else
		{
			success = true;
			if (MessageBox(m_hWnd, szErrStr, WindowTitle, MB_OKCANCEL|MB_ICONWARNING) == IDCANCEL)
			{
				success = false;
				break;
			}
		}
	}
	
	sprintf(szErrStr, "Files successfully exported: %d", n);
	MessageBox(m_hWnd, szErrStr, WindowTitle, MB_OK|MB_ICONINFORMATION);
*/
}


// File import
void BeebWin::ImportDiscFiles(int menuId)
{
	OSErr err = noErr;
	char path[256];
	bool success = true;
	int drive;
	int type;
	char szDiscFile[MAX_PATH];
	int heads;
	int side;
	char szErrStr[500];
	char szFolder[MAX_PATH];
	char fileSelection[4096];
	char baseName[MAX_PATH];
	char *fileName;
	static char fileNames[DFS_MAX_CAT_SIZE*2][MAX_PATH]; // Allow user to select > cat size
	int numFiles;
	int i, n;
	
	if (menuId == 0 || menuId == 2)
		drive = 0;
	else
		drive = 1;
	
	if (MachineType != 3 && NativeFDC)
	{
		// 8271 controller
		Get8271DiscInfo(drive, szDiscFile, &heads);
	}
	else
	{
		// 1770 controller
		Get1770DiscInfo(drive, &type, szDiscFile);
		if (type == 0)
			heads = 1;
		else if (type == 1)
			heads = 2;
		else
		{
			// ADFS - not currently supported
			fprintf(stderr, "Import to ADFS disc not supported\n");
			return;
		}
	}
	
	// Check for no disk loaded
	if (szDiscFile[0] == 0 || heads == 1 && (menuId == 2 || menuId == 3))
	{
		fprintf(stderr, "No disc loaded in drive %d\n", menuId);
		return;
	}
	
	// Read the catalogue
	if (menuId == 0 || menuId == 1)
		side = 0;
	else
		side = 1;
	
	success = dfs_get_catalogue(szDiscFile, heads, side, &dfsCat);
	if (!success)
	{
		fprintf(stderr, "Failed to read catalogue from disc:\n  %s\n", szDiscFile);
		return;
	}
	
	// Get list of files to import
	err = GetOneFileWithPreview(path, IFDFilterProc);
	if (err) return;
	
	// Parse the file selection string
	strcpy(fileSelection, path);
	// Only one file selected
	fileName = strrchr(fileSelection, '/');
	if (fileName != NULL)
	{
		*fileName = 0;
		fileName++;
	} else fileName = fileSelection;
	
	strcpy(szFolder, fileSelection);
	
	numFiles = 0;
	while (numFiles < DFS_MAX_CAT_SIZE*2 && fileName[0] != 0)
	{
		// Strip .INF off
		strcpy(baseName, fileName);
		i = (int)strlen(baseName);
		if (i > 4 && strcmp(baseName + i - 4, ".inf") == 0)
			baseName[i - 4] = 0;
		
		// Check for duplicate
		for (i = 0; i < numFiles; ++i)
		{
			if (strcmp(baseName, fileNames[i]) == 0)
				break;
		}
		if (i == numFiles)
		{
			strcpy(fileNames[numFiles], baseName);
			numFiles++;
		}
		
		fileName = fileName + strlen(fileName) + 1;
	}
	
	// Import the files
	n = 0;
	for (i = 0; i < numFiles; ++i)
	{
		success = dfs_import_file(szDiscFile, heads, side, &dfsCat, fileNames[i], szFolder, szErrStr);
		if (success)
		{
			n++;
		}
		else
		{
			success = true;
			fprintf(stderr, "%s\n", szErrStr);
			success = false;
			break;
		}
	}
	
	fprintf(stderr, "Files successfully imported: %d\n", n);
	
	// Re-read disc image
	if (MachineType != 3 && NativeFDC)
	{
		// 8271 controller
		Eject8271DiscImage(drive);
		if (heads == 2)
			LoadSimpleDSDiscImage(szDiscFile, drive, 80);
		else
			LoadSimpleDiscImage(szDiscFile, drive, 0, 80);
	}
	else
	{
		// 1770 controller
		Close1770Disc(drive);
		if (heads == 2)
			Load1770DiscImage(szDiscFile, drive, 1);
		else
			Load1770DiscImage(szDiscFile, drive, 0);
	}
}

