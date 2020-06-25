/****************************************************************************/
/*                               Beebem                                     */
/*                               ------                                     */
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
/****************************************************************************/
/* User defined keyboard funcitonality - Laurie Whiffen 26/8/97 */

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "userkybd.h"
#include "plist.h"

void GetKeysUsed( char *Keys );
const char *KeyName( int Key );

int		BBCRow;			// Used to store the Row and Col values while we wait 
int		BBCCol;			// for a key press from the User.

WindowRef mUKWindow = NULL; 

// Row,Col  Default values set to transTable1
int UserKeymap[256][2]={
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

int LastButton = 0;

void ShowKey(int key)
{
char Keys[256];
CFStringRef pTitle;

	GetKeysUsed(Keys);

//	fprintf(stderr, "%s\n", Keys);

	const ControlID dbControlID = { 'ass ', 0 };
	ControlRef dbControl;
	
	GetControlByID (mUKWindow, &dbControlID, &dbControl);

	pTitle = CFStringCreateWithCString (kCFAllocatorDefault, Keys, kCFStringEncodingASCII);

    SetControlData(dbControl, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &pTitle);
	
	CFRelease(pTitle);
	if ( (LastButton != 0) && (LastButton != key) )
	{

		ControlID dbKeyID;
		
		dbKeyID.signature = LastButton;
		dbKeyID.id = 0;
		
		GetControlByID (mUKWindow, &dbKeyID, &dbControl);
		SetControlValue(dbControl, 0);
	}
	LastButton = key;
}

//*******************************************************************

OSStatus UKWindowCommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
	int i;
    HICommand command; 
    OSStatus err = noErr;
    err = GetEventParameter(event, kEventParamDirectObject,
							typeHICommand, NULL, sizeof(HICommand), NULL, &command);
    require_noerr (err, CantGetParameter);
	
	err = noErr;

	SetRowCol(command.commandID);
	
	if ((BBCRow == 0) & (BBCCol == 0))
	{
		switch (command.commandID)
		{
			case 'ok  ':
				UserKeyboardCloseDialog();
				break;
				
			case 'rest':
				for (i = 0; i < 256; ++i)
				{
					UserKeymap[i][0] = transTable1[i][0];
					UserKeymap[i][1] = transTable1[i][1];
				}
				if (LastButton != 0)
				{
					SetRowCol(LastButton);
					ShowKey(LastButton);
				}
				break;
				
			default:
				err = eventNotHandledErr;
				break;
		}
	}
	else
	{
		ShowKey(command.commandID);
	}

CantGetParameter:
		return err;
}

static OSStatus UKWindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    OSStatus err = noErr;
	char charCode;
	int keycode;

	switch (GetEventKind(event)) 
	{
		case kEventRawKeyDown:
			GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
			GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(int), NULL, &keycode);
			fprintf(stderr, "Key pressed: code = %d, '%c'\n", keycode, charCode);
			if (LastButton != 0)
			{
				SetBBCKeyForVKEY(keycode);
				ShowKey(LastButton);
			}
			break;

        case kEventWindowClosed: 
			mUKWindow = NULL;
            break;
        
		default:
            err = eventNotHandledErr;
            break;
    }
    
	return err;
}

void UserKeyboardOpenDialog()
{
	IBNibRef 		nibRef;
	EventTypeSpec UKcommands[] = {
	{ kEventClassCommand, kEventCommandProcess }
	};
		
	EventTypeSpec UKevents[] = {
	{ kEventClassWindow, kEventWindowClosed },
	{ kEventClassKeyboard, kEventRawKeyDown}
	};

	if (mUKWindow == NULL)
	{
		// Create a Nib reference passing the name of the nib file (without the .nib extension)
		// CreateNibReference only searches into the application bundle.
		CreateNibReference(CFSTR("main"), &nibRef);
		CreateWindowFromNib(nibRef, CFSTR("Window1"), &mUKWindow);
		DisposeNibReference(nibRef);
		ShowWindow(mUKWindow);
		
		InstallWindowEventHandler(mUKWindow, 
							  NewEventHandlerUPP (UKWindowCommandHandler), 
							  GetEventTypeCount(UKcommands), UKcommands, 
							  mUKWindow, NULL);
		
		InstallWindowEventHandler (mUKWindow, 
								   NewEventHandlerUPP (UKWindowEventHandler), 
								   GetEventTypeCount(UKevents), UKevents, 
								   mUKWindow, NULL);
		
	}
}

void UserKeyboardCloseDialog()
{
	if (mUKWindow)
	{
		HideWindow(mUKWindow);
		DisposeWindow(mUKWindow);
	}
	mUKWindow = NULL;
}

/****************************************************************************/
void SetBBCKeyForVKEY( int Key )
{

	UserKeymap[ Key ] [0] = BBCRow;
	UserKeymap[ Key ] [1] = BBCCol;

} // SetBBCKeyForVKEY

/****************************************************************************/

void GetKeysUsed( char *Keys )
{
	int i;

	Keys[0] = '\0';

	// First see if this key is defined.
	if ((BBCRow == 0 ) && (BBCCol == 0 ))
		strcpy( Keys, "Not Assigned" );
	else
	{
		for( i=0; i<256; i++ )
		{
			if ((UserKeymap[i][0] == BBCRow) &&
				(UserKeymap[i][1] == BBCCol ))
			{  // We have found a key that matches.
				if (strlen( Keys ) != 0 )
					strcat( Keys,  ", " );
				strcat( Keys, KeyName(i) );

			}
		}
		if ( strlen( Keys ) == 0 )
			strcpy( Keys, "Not Assigned" );
	}

}

/****************************************************************************/

const char *KeyName( int Key )
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
	case  64: return "F17";
	case  65: return "Pad.";
	case  67: return "Pad*";
	case  69: return "Pad+";
	case  71: return "Clear";
	case  75: return "Pad/";			
	case  76: return "Enter";
	case  78: return "Pad-";
	case  79: return "F18";
	case  80: return "F19";
	case  81: return "Pad=";
	case  82: return "Pad0";
	case  83: return "Pad1";
	case  84: return "Pad2";
	case  85: return "Pad3";
	case  86: return "Pad4";
	case  87: return "Pad5";
	case  88: return "Pad6";
	case  89: return "Pad7";
	case  91: return "Pad8";
	case  92: return "Pad9";
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
		return "Unknown";
	}

}

/****************************************************************************/

void SetRowCol( int ctrlID )
{
	switch( ctrlID )
	{
	// Character keys.
	case 'A   ': BBCRow = 4; BBCCol = 1; break;
	case 'B   ': BBCRow = 6; BBCCol = 4; break;
	case 'C   ': BBCRow = 5; BBCCol = 2; break;
	case 'D   ': BBCRow = 3; BBCCol = 2; break;
	case 'E   ': BBCRow = 2; BBCCol = 2; break;
	case 'F   ': BBCRow = 4; BBCCol = 3; break;
	case 'G   ': BBCRow = 5; BBCCol = 3; break;
	case 'H   ': BBCRow = 5; BBCCol = 4; break;
	case 'I   ': BBCRow = 2; BBCCol = 5; break;
	case 'J   ': BBCRow = 4; BBCCol = 5; break;
	case 'K   ': BBCRow = 4; BBCCol = 6; break;
	case 'L   ': BBCRow = 5; BBCCol = 6; break;
	case 'M   ': BBCRow = 6; BBCCol = 5; break;
	case 'N   ': BBCRow = 5; BBCCol = 5; break;
	case 'O   ': BBCRow = 3; BBCCol = 6; break;
	case 'P   ': BBCRow = 3; BBCCol = 7; break;
	case 'Q   ': BBCRow = 1; BBCCol = 0; break;
	case 'R   ': BBCRow = 3; BBCCol = 3; break;
	case 'S   ': BBCRow = 5; BBCCol = 1; break;
	case 'T   ': BBCRow = 2; BBCCol = 3; break;
	case 'U   ': BBCRow = 3; BBCCol = 5; break;
	case 'V   ': BBCRow = 6; BBCCol = 3; break;
	case 'W   ': BBCRow = 2; BBCCol = 1; break;
	case 'X   ': BBCRow = 4; BBCCol = 2; break;
	case 'Y   ': BBCRow = 4; BBCCol = 4; break;
	case 'Z   ': BBCRow = 6; BBCCol = 1; break;
		
	// Number keys.
	case '0   ': BBCRow = 2; BBCCol = 7; break;
	case '1   ': BBCRow = 3; BBCCol = 0; break;
	case '2   ': BBCRow = 3; BBCCol = 1; break;
	case '3   ': BBCRow = 1; BBCCol = 1; break;
	case '4   ': BBCRow = 1; BBCCol = 2; break;
	case '5   ': BBCRow = 1; BBCCol = 3; break;
	case '6   ': BBCRow = 3; BBCCol = 4; break;
	case '7   ': BBCRow = 2; BBCCol = 4; break;
	case '8   ': BBCRow = 1; BBCCol = 5; break;
	case '9   ': BBCRow = 2; BBCCol = 6; break;

	// Function keys.
	case 'F0  ': BBCRow = 2; BBCCol = 0; break;
	case 'F1  ': BBCRow = 7; BBCCol = 1; break;
	case 'F2  ': BBCRow = 7; BBCCol = 2; break;
	case 'F3  ': BBCRow = 7; BBCCol = 3; break;
	case 'F4  ': BBCRow = 1; BBCCol = 4; break;
	case 'F5  ': BBCRow = 7; BBCCol = 4; break;
	case 'F6  ': BBCRow = 7; BBCCol = 5; break;
	case 'F7  ': BBCRow = 1; BBCCol = 6; break;
	case 'F8  ': BBCRow = 7; BBCCol = 6; break;
	case 'F9  ': BBCRow = 7; BBCCol = 7; break;

	// Special keys.
	case 'LFT ': BBCRow = 1; BBCCol = 9; break;
	case 'RGT ': BBCRow = 7; BBCCol = 9; break;
	case 'UP  ': BBCRow = 3; BBCCol = 9; break;
	case 'DN  ': BBCRow = 2; BBCCol = 9; break;
	case 'BRK ': BBCRow = -2; BBCCol = -2; break;
	case 'COPY': BBCRow = 6; BBCCol = 9; break;
	case 'DEL ': BBCRow = 5; BBCCol = 9; break;
	case 'CAPS': BBCRow = 4; BBCCol = 0; break;
	case 'TAB ': BBCRow = 6; BBCCol = 0; break;
	case 'CTRL': BBCRow = 0; BBCCol = 1; break;
	case 'SPC ': BBCRow = 6; BBCCol = 2; break;
	case 'RET ': BBCRow = 4; BBCCol = 9; break;
	case 'ESC ': BBCRow = 7; BBCCol = 0; break;
	case 'LSFT': BBCRow = 0; BBCCol = 0; break;
	case 'RSHT': BBCRow = 0; BBCCol = 0; break;
	case 'SLCK': BBCRow = 5; BBCCol = 0; break;

	//Special Character keys.
	case ';+  ': BBCRow = 5; BBCCol = 7; break;
	case '-=  ': BBCRow = 1; BBCCol = 7; break;
	case ',<  ': BBCRow = 6; BBCCol = 6; break;
	case '^~  ': BBCRow = 1; BBCCol = 8; break;
	case '.>  ': BBCRow = 6; BBCCol = 7; break;
	case '/?  ': BBCRow = 6; BBCCol = 8; break;
	case ':*  ': BBCRow = 4; BBCCol = 8; break;
	case '[{  ': BBCRow = 3; BBCCol = 8; break;
	case '\\|  ': BBCRow = 7; BBCCol = 8; break;
	case ']}  ': BBCRow = 5; BBCCol = 8; break;
	case '@   ': BBCRow = 4; BBCCol = 7; break;
	case '_�  ': BBCRow = 2; BBCCol = 8; break;
	
	default:
		BBCRow = 0; BBCCol = 0;
	}

//	fprintf(stderr, "Key %08x, Row = %d, Col = %d\n", ctrlID, BBCRow, BBCCol);

}

bool LoadUserKeyboard( const char *path )
{
	CFStringRef keyboardFile = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingASCII);
	CFURLRef	keyboardFileUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,    
																keyboardFile,			// file path name
																kCFURLPOSIXPathStyle,	// interpret as POSIX path        
																false);					// is it a directory?
	
	if (keyboardFileUrl == NULL)
	{
		fprintf(stderr, "Cannot create keyboard file URL for file %s\n", path);
		return false;
	}
	
	CFMutableDictionaryRef dict = (CFMutableDictionaryRef) CreateMyPropertyListFromFile(keyboardFileUrl);
	
	if (dict == NULL)
	{
		fprintf(stderr, "Cannot create property file\n");
		return false;
	}
	
	for (int i = 0; i < 256; ++i)
	{		
		char key [256];
		sprintf(key, "Row%d", i);
		CFStringRef keyRef = CFStringCreateWithCString (kCFAllocatorDefault, key, kCFStringEncodingASCII);
		UserKeymap[i][0] = GetDictNum(dict, keyRef, transTable1[i][0]);
		CFRelease(keyRef);
		
		sprintf(key, "Col%d", i);
		keyRef = CFStringCreateWithCString (kCFAllocatorDefault, key, kCFStringEncodingASCII);
		UserKeymap[i][1] = GetDictNum(dict, keyRef, transTable1[i][1]);
		CFRelease(keyRef);
	}
	
	return true;
}

void SaveUserKeyboard( char *path )
{
	// Create a dictionary for the keyboard mappings.
	CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault,
															0,
															&kCFTypeDictionaryKeyCallBacks,
															&kCFTypeDictionaryValueCallBacks);
	
	if (dict == NULL)
	{
		fprintf(stderr, "Cannot create property file\n");
		return;
	}
	
	// Put the keymap rows/columns into the dictionary.
	for (int i = 0; i < 256; ++i)
	{		
		char key [256];
		sprintf(key, "Row%d", i);
		CFStringRef keyRef = CFStringCreateWithCString (kCFAllocatorDefault, key, kCFStringEncodingASCII);
		AddDictNum(dict, keyRef, UserKeymap[i][0]);
		CFRelease(keyRef);
		
		sprintf(key, "Col%d", i);
		keyRef = CFStringCreateWithCString (kCFAllocatorDefault, key, kCFStringEncodingASCII);
		AddDictNum(dict, keyRef, UserKeymap[i][1]);
		CFRelease(keyRef);
	}
	
	// create a URL for the keymap file to be saved
	CFStringRef keyboardFile = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingASCII);
	CFURLRef	keyboardFileUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,    
																keyboardFile,			// file path name
																kCFURLPOSIXPathStyle,	// interpret as POSIX path        
																false);					// is it a directory?
	
	WriteMyPropertyListToFile(dict, keyboardFileUrl);
	
	CFRelease(dict);
	CFRelease(keyboardFileUrl);
	CFRelease(keyboardFile);
}
