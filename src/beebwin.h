/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 1994  Nigel Magnay
Copyright (C) 1997  Mike Wyatt
Copyright (C) 1998  Robert Schmidt
Copyright (C) 2001  Richard Gellman
Copyright (C) 2004  Ken Lowe
Copyright (C) 2004  Rob O'Donnell
Copyright (C) 2005  Jon Welch

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

/* Mike Wyatt and NRM's port to win32 - 07/06/1997 */

#ifndef BEEBWIN_HEADER
#define BEEBWIN_HEADER

#include <string.h>
#include <stdlib.h>
#include <string>

#ifdef BEEBWIN
#include <windows.h>
#include <d3dx9.h>
#include <ddraw.h>
#include <sapi.h>
#else

#define UINT uint32_t
#define CHAR char

// from Carbon
struct RECT {
  short               top;
  short               left;
  short               bottom;
  short               right;
};
typedef struct RECT                     RECT;

// 16 rows and 2 columns (only column 0 needs the bank value)
struct RCItem {
    int bank;
    std::string name;
};

#endif

#include "disctype.h"
#include "model.h"
#include "port.h"
#include "preferences.h"
#include "video.h"

/* Used in message boxes */
#define GETHWND (mainWin->GethWnd())

// Registry defs for disabling windows keys
#define CFG_KEYBOARD_LAYOUT "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout"
#define CFG_SCANCODE_MAP "Scancode Map"

typedef struct KeyMappingTag {
	int row;    // Beeb row
	int col;    // Beeb col
	bool shift; // Beeb shift state
} KeyMapping;

typedef KeyMapping bKeyMap[256][2]; // Indices are: [Virt key][shift state]


extern const char *WindowTitle;

typedef union EightUChars {
	unsigned char data[8];
	EightByteType eightbyte;
} EightUChars;

typedef union SixteenUChars {
	unsigned char data[16];
	EightByteType eightbytes[2];
} SixteenUChars;

#ifndef BEEBWIN
// placeholder
#define LONG u_int32_t
typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved;
} RGBQUAD;
#endif

typedef struct bmiData
{
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD			bmiColors[256];
} bmiData;

struct LEDType {
	bool ShiftLock;
	bool CapsLock;
	bool Motor;
	bool Disc0;
	bool Disc1;
	bool HDisc[4];
	bool ShowDisc;
	bool ShowKB;
};
extern struct LEDType LEDs;

enum class LEDColour {
	Red,
	Green
};

extern LEDColour DiscLedColour;

enum TextToSpeechSearchDirection
{
	TTS_FORWARDS,
	TTS_BACKWARDS
};

enum TextToSpeechSearchType
{
	TTS_CHAR,
	TTS_BLANK,
	TTS_NONBLANK,
	TTS_ENDSENTENCE
};

#ifdef BEEBWIN
// A structure for our custom vertex type. We added texture coordinates
struct CUSTOMVERTEX
{
	D3DXVECTOR3 position; // The position
	D3DCOLOR	color;	  // The color
	FLOAT		tu, tv;	  // The texture coordinates
};
#endif

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

enum class MessageType {
	Error,
	Warning,
	Info,
	Question,
	Confirm
};

enum class MessageResult {
	None,
	Yes,
	No,
	OK,
	Cancel
};

enum class PaletteType : char {
	RGB,
	BW,
	Amber,
	Green,
	Last
};

class BeebWin {

public:
	BeebWin();
	~BeebWin();

	bool Initialise();
	void ApplyPrefs();
	void Shutdown();

	void UpdateModelMenu();
	void SetSoundMenu(void);
	void SetImageName(const char *DiscName, int Drive, DiscType DscType);
	void SetTapeSpeedMenu(void);
	void SetDiscWriteProtects(void);
	void SetRomMenu(void);				// LRW  Added for individual ROM/Ram
	void UpdateTubeMenu();
	void SelectFDC();
	void LoadFDC(char *DLLName, bool save);
	void KillDLLs(void);
	void UpdateLEDMenu();
	void SetDriveControl(unsigned char value);
	unsigned char GetDriveControl(void);
	void doLED(int sx,bool on);
#ifdef BEEBWIN
	void updateLines(HDC hDC, int starty, int nlines);
    void updateLines(int starty, int nlines) {
		updateLines(m_hDC, starty, nlines);
    }
#else
    void updateLines(int starty, int nlines);
    void Blt(RECT destR, RECT srcR);
    
    int_fast32_t m_RGB32[256];
    int_fast16_t m_RGB16[256];
#endif

	void doHorizLine(int Colour, int y, int sx, int width) {
		if (TeletextEnabled) y/=TeletextStyle;
		int d = (y*800)+sx+ScreenAdjust+(TeletextEnabled?36:0);
		if ((d+width)>(500*800)) return;
		if (d<0) return;
		memset(m_screen+d, Colour, width);
	}

	void doInvHorizLine(int Colour, int y, int sx, int width) {
		if (TeletextEnabled) y/=TeletextStyle;
		int d = (y*800)+sx+ScreenAdjust+(TeletextEnabled?36:0);
		char *vaddr;
		if ((d+width)>(500*800)) return;
		if (d<0) return;
		vaddr=m_screen+d;
		for (int n = 0; n < width; n++) *(vaddr+n) ^= Colour;
	}

	void doUHorizLine(int Colour, int y, int sx, int width) {
		if (TeletextEnabled) y/=TeletextStyle;
		if (y>500) return;
		memset(m_screen+ (y* 800) + sx, Colour, width);
	}

	EightUChars *GetLinePtr(int y) {
		int d = (y*800)+ScreenAdjust;
		if (d > (MAX_VIDEO_SCAN_LINES*800))
			return((EightUChars *)(m_screen+(MAX_VIDEO_SCAN_LINES*800)));
		return((EightUChars *)(m_screen + d));
	}

	SixteenUChars *GetLinePtr16(int y) {
		int d = (y*800)+ScreenAdjust;
		if (d > (MAX_VIDEO_SCAN_LINES*800))
			return((SixteenUChars *)(m_screen+(MAX_VIDEO_SCAN_LINES*800)));
		return((SixteenUChars *)(m_screen + d));
	}

#ifdef BEEBWIN
	HWND GethWnd() { return m_hWnd; }

	void RealizePalette(HDC) {};
#endif
    void ResetBeebSystem(Model NewModelType, bool LoadRoms);

	void CreateArmCoPro();
	void DestroyArmCoPro();
	void CreateSprowCoPro();
	void DestroySprowCoPro();

	int StartOfFrame(void);
	bool UpdateTiming();
	void AdjustSpeed(bool up);
	bool ShouldDisplayTiming() const;
	void DisplayTiming();
	void UpdateWindowTitle();
	bool IsWindowMinimized() const;
#ifdef BEEBWIN

	void DisplayClientAreaText(HDC hdc);
	void DisplayFDCBoardInfo(HDC hDC, int x, int y);
#endif
    void ScaleJoystick(unsigned int x, unsigned int y);
	void SetMousestickButton(int index, bool button);
	void ScaleMousestick(unsigned int x, unsigned int y);
	void HandleCommand(int MenuId);
	void SetAMXPosition(unsigned int x, unsigned int y);
	void ChangeAMXPosition(int deltaX, int deltaY);
	void CaptureMouse();
	void ReleaseMouse();
	void Activate(bool active);
	void Focus(bool gotit);
#ifdef BEEBWIN

	void WinSizeChange(WPARAM size, int width, int height);
#endif
    void WinPosChange(int x, int y);
	bool IsFrozen();
	void TogglePause();
	bool IsPaused();
	void OpenUserKeyboardDialog();
	void UserKeyboardDialogClosed();
	void ShowMenu(bool on);
	void HideMenu(bool hide);
	void TrackPopupMenu(int x, int y);
	bool IsFullScreen() { return m_isFullScreen; }
	void ResetTiming(void);
	int TranslateKey(int vkey, bool keyUp, int &row, int &col);
	void ParseCommandLine(void);
	void CheckForLocalPrefs(const char *path, bool bLoadPrefs);
	void FindCommandLineFile(char *CmdLineFile);
	void HandleCommandLineFile(int drive, const char *CmdLineFile);
	void HandleEnvironmentVariables();
	void LoadStartupDisc(int DriveNum, const char *DiscString);
	bool CheckUserDataPath(bool Persist);
	void SelectUserDataPath(void);
	void StoreUserDataPath(void);
	bool NewTapeImage(char *FileName);
	const char *GetAppPath(void) { return m_AppPath; }
	const char *GetUserDataPath(void) { return m_UserDataPath; }
	void GetDataPath(const char *folder, char *path);
	void QuickLoad();
	void QuickSave();
	void LoadUEFState(const char *FileName);
	void SaveUEFState(const char *FileName);
	void LoadUEFTape(const char *FileName);
	void LoadCSWTape(const char *FileName);

	void Speak(const char *text, DWORD flags);
	void SpeakChar(unsigned char c);
#ifdef BEEBWIN

	void TextToSpeechKey(WPARAM uParam);
#endif
    void TextViewSpeechSync(void);
	void TextViewSyncWithBeebCursor(void);
	void HandleTimer(void);
	void doCopy(void);
	void doPaste(void);
	void ClearClipboardBuffer();
	void CopyKey(unsigned char Value);
	void CaptureBitmapPending(bool autoFilename);
	void DoShiftBreak();
	bool HasKbdCmd() const;
	void SetKeyboardTimer();
	void SetBootDiscTimer();
	void KillBootDiscTimer();

	void SaveEmuUEF(FILE *SUEF);
	void LoadEmuUEF(FILE *SUEF,int Version);

#ifdef BEEBWIN

	HMENU		m_hMenu;
#endif
    bool		m_frozen;
	char*		m_screen;
	char*		m_screen_blur;
	double		m_RealTimeTarget;
	bool		m_ShiftBooted;
	bool		m_TextToSpeechEnabled;
	bool		m_TextViewEnabled;
	bool		m_DisableKeysWindows;
	bool		m_DisableKeysBreak;
	bool		m_DisableKeysEscape;
	bool		m_DisableKeysShortcut;

	int		m_MenuIdWinSize;
	int		m_XWinSize;
	int		m_YWinSize;
	int		m_XLastWinSize;
	int		m_YLastWinSize;
	int		m_XWinPos;
	int		m_YWinPos;
	int		m_XDXSize;
	int		m_YDXSize;
	int		m_XScrSize;
	int		m_YScrSize;
	int		m_XWinBorder;
	int		m_YWinBorder;
	float		m_XRatioAdj;
	float		m_YRatioAdj;
	float		m_XRatioCrop;
	float		m_YRatioCrop;
	bool		m_ShowSpeedAndFPS;
	int		m_MenuIdSampleRate;
	int		m_MenuIdVolume;
	int		m_MenuIdTiming;
	int		m_FPSTarget;
	bool		m_JoystickCaptured;
#ifdef BEEBWIN

	JOYCAPS		m_JoystickCaps;
#endif
	int		m_MenuIdSticks;
	bool		m_HideCursor;
	bool		m_CaptureMouse;
	bool		m_MouseCaptured;
#ifdef BEEBWIN
	POINT		m_RelMousePos;
#endif
    bool		m_FreezeWhenInactive;
	int		m_MenuIdKeyMapping;
	bool		m_KeyMapAS;
	bool		m_KeyMapFunc;
	char		m_UserKeyMapPath[_MAX_PATH];
	bool		m_ShiftPressed;
	int		m_vkeyPressed[256][2][2];
	char		m_AppPath[_MAX_PATH];
	char		m_UserDataPath[_MAX_PATH];
	bool m_CustomData;
	char		m_DiscPath[_MAX_PATH];	// JGH
	bool		m_WriteProtectDisc[2];
	bool		m_WriteProtectOnLoad;
	int		m_MenuIdAMXSize;
	int		m_MenuIdAMXAdjust;
	int		m_AMXXSize;
	int		m_AMXYSize;
	int		m_AMXAdjust;
	int		m_DisplayRenderer;
	int		m_CurrentDisplayRenderer;
	int		m_DDFullScreenMode;
	bool		m_isFullScreen;
	bool		m_MaintainAspectRatio;
	bool		m_startFullScreen;
	bool		m_AutoSavePrefsCMOS;
	bool		m_AutoSavePrefsFolders;
	bool		m_AutoSavePrefsAll;
	bool		m_AutoSavePrefsChanged;
	bool m_HideMenuEnabled;
	bool m_DisableMenu;
	bool m_MenuOn;

	char		m_customip [20];		//IP232
	int		m_customport;

#ifdef BEEBWIN

	HDC 		m_hDC;
	HWND		m_hWnd;
	HGDIOBJ 	m_hOldObj;
	HDC 		m_hDCBitmap;
	HGDIOBJ 	m_hBitmap;
#endif
	bmiData 	m_bmi;
    char m_szTitle[256];

	int		m_ScreenRefreshCount;
	double		m_RelativeSpeed;
	double		m_FramesPerSecond;

	static const int ClipboardBufferSize = 32768;

	char m_ClipboardBuffer[ClipboardBufferSize];
	int m_ClipboardLength;
	int m_ClipboardIndex;

	char		m_printerbuffer[1024 * 1024];
	int		m_printerbufferlen;
	bool		m_translateCRLF;

	int		m_MenuIdPrinterPort;
	char		m_PrinterFileName[_MAX_PATH];
	char		m_PrinterDevice[_MAX_PATH];

	DWORD		m_LastTickCount;
	DWORD		m_LastStatsTickCount;
	int		m_LastTotalCycles;
	int		m_LastStatsTotalCycles;
	DWORD		m_TickBase;
	int		m_CycleBase;
	int		m_MinFrameCount;
	DWORD		m_LastFPSCount;
	int		m_LastStartY;
	int		m_LastNLines;
	int		m_MotionBlur;
	char 		m_BlurIntensities[8];
	char 		m_CommandLineFileName1[_MAX_PATH];
	char 		m_CommandLineFileName2[_MAX_PATH];
	char		m_KbdCmd[1024];
	char		m_DebugScript[_MAX_PATH];
	std::string m_DebugLabelsFileName;
	int		m_KbdCmdPos;
	int		m_KbdCmdKey;
	bool		m_KbdCmdPress;
	int		m_KbdCmdDelay;
	int		m_KbdCmdLastCycles;
	bool		m_NoAutoBoot;
	int		m_AutoBootDelay;
	bool		m_EmuPaused;
	bool		m_StartPaused;
	bool		m_WasPaused;
	bool		m_AutoBootDisc;
	bool		m_KeyboardTimerElapsed;
	bool		m_BootDiscTimerElapsed;
	unsigned char RomWritePrefs[16];

	// Bitmap capture vars
#ifdef BEEBWIN
	ULONG_PTR	m_gdiplusToken;
#endif
	bool		m_CaptureBitmapPending;
	bool		m_CaptureBitmapAutoFilename;
	char		m_CaptureFileName[MAX_PATH];
	int		m_MenuIdCaptureResolution;
	int		m_MenuIdCaptureFormat;

	// AVI vars
#ifdef BEEBWIN
	bmiData 	m_Avibmi;
	HBITMAP		m_AviDIB;
    HDC 		m_AviDC;
#endif
	char*		m_AviScreen;
	int		m_AviFrameSkip;
	int		m_AviFrameSkipCount;
	int		m_AviFrameCount;
	int		m_MenuIdAviResolution;
	int		m_MenuIdAviSkip;

	// DirectX stuff
	bool		m_DXInit;
	bool		m_DXResetPending;

#ifdef BEEBWIN
	// DirectDraw stuff
	LPDIRECTDRAW		m_DD;			// DirectDraw object
	LPDIRECTDRAW2		m_DD2;			// DirectDraw object
	LPDIRECTDRAWSURFACE	m_DDSPrimary;	// DirectDraw primary surface
	LPDIRECTDRAWSURFACE2	m_DDS2Primary;	// DirectDraw primary surface
	LPDIRECTDRAWSURFACE	m_DDSOne;		// Offscreen surface 1
	LPDIRECTDRAWSURFACE2	m_DDS2One;		// Offscreen surface 1
	bool			m_DXSmoothing;
	bool			m_DXSmoothMode7Only;
	LPDIRECTDRAWCLIPPER	m_Clipper;		// clipper for primary

	// Direct3D9 stuff
	LPDIRECT3D9             m_pD3D;
	LPDIRECT3DDEVICE9       m_pd3dDevice;
	LPDIRECT3DVERTEXBUFFER9 m_pVB;
	LPDIRECT3DTEXTURE9      m_pTexture;
	D3DXMATRIX              m_TextureMatrix;
    
	// Text to speech variables
	ISpVoice *m_SpVoice;
#endif
	int m_SpeechLine;
	int m_SpeechCol;
	static const int MAX_SPEECH_LINE_LEN = 128;
	static const int MAX_SPEECH_SENTENCE_LEN = 128*25;
	static const int MAX_SPEECH_SCREEN_LEN = 128*32;
	char m_SpeechText[MAX_SPEECH_LINE_LEN+1];
	bool m_SpeechSpeakPunctuation;
	bool m_SpeechWriteChar;
	static const int MAX_SPEECH_BUF_LEN = 160;
	char m_SpeechBuf[MAX_SPEECH_BUF_LEN+1];
	int m_SpeechBufPos;

	// Text view variables
#ifdef BEEBWIN
	HWND m_hTextView;
#endif
    static const int MAX_TEXTVIEW_SCREEN_LEN = 128*32;
	char m_TextViewScreen[MAX_TEXTVIEW_SCREEN_LEN+1];

	bool InitClass();
	void UpdateOptiMenu();
	void CreateBeebWindow(void);
	void CreateBitmap(void);
	void InitMenu();
	void UpdateMonitorMenu();
	void SelectSerialPort(unsigned char PortNumber);
	void UpdateSerialMenu();
	void UpdateEconetMenu();

	void UpdateSFXMenu();
	void UpdateDisableKeysMenu();
	void UpdateDisplayRendererMenu();

	void UpdateSoundStreamerMenu();

	void CheckMenuItem(UINT id, bool checked);
	void EnableMenuItem(UINT id, bool enabled);
    
	// DirectX - calls DDraw or DX9 fn
	void InitDX(void);
	void ResetDX(void);
	void ReinitDX(void);
	void ExitDX(void);
	void UpdateSmoothing(void);

#ifdef BEEBWIN
	// DirectDraw
	HRESULT InitDirectDraw(void);
	HRESULT InitSurfaces(void);
    void ResetSurfaces(void);

	// DirectX9
	HRESULT InitDX9(void);
	void ExitDX9(void);
	void RenderDX9(void);
#endif

	void TranslateWindowSize(void);
	void TranslateDDSize(void);
	void CalcAspectRatioAdjustment(int DisplayWidth, int DisplayHeight);
	void TranslateSampleRate(void);
	void TranslateVolume(void);
	void TranslateTiming(void);
	void TranslateKeyMapping(void);
	int ReadDisc(int Drive, bool bCheckForPrefs);
	void Load1770DiscImage(const char *FileName, int Drive, DiscType Type);
	void LoadTape(void);
	void InitJoystick(void);
	void ResetJoystick(void);
	void RestoreState(void);
	void SaveState(void);
	void NewDiscImage(int Drive);
	void CreateDiscImage(const char *FileName, int DriveNum, int Heads, int Tracks);
	void EjectDiscImage(int Drive);
	void ExportDiscFiles(int menuId);
    void ExportDiscFilesToFolder();
	void ImportDiscFiles(int menuId);
	void SelectHardDriveFolder();
	void ToggleWriteProtect(int Drive);
	void SetWindowAttributes(bool wasFullScreen);
	void TranslateAMX(void);
	bool PrinterFile();
	void TogglePrinter(void);
	void TranslatePrinterPort(void);
	void CaptureVideo();
	void EndVideo();
	void CaptureBitmap(int SourceX,
	                   int SourceY,
	                   int SourceWidth,
	                   int SourceHeight,
	                   bool Teletext);
	bool GetImageFile(char *FileName);
#ifdef BEEBWIN
    bool GetImageEncoderClsid(const WCHAR *mimeType, CLSID *encoderClsid);
#endif
	void InitTextToSpeech(void);
	bool TextToSpeechSearch(TextToSpeechSearchDirection dir,
							TextToSpeechSearchType type);
	void TextToSpeechReadChar(void);
	void TextToSpeechReadWord(void);
	void TextToSpeechReadLine(void);
	void TextToSpeechReadSentence(void);
	void TextToSpeechReadScreen(void);
	void InitTextView(void);
	void TextView(void);
	void TextViewSetCursorPos(int line, int col);
	bool RebootSystem();
	void LoadUserKeyMap(void);
	void SaveUserKeyMap(void);

	MessageResult Report(MessageType type, const char *format, ...);

    bool ReadKeyMap(const char *filename, bKeyMap *keymap);
    bool WriteKeyMap(const char *filename, bKeyMap *keymap);
#ifdef BEEBWIN
	bool RegCreateKey(HKEY hKeyRoot, LPCSTR lpSubKey);
	bool RegGetBinaryValue(HKEY hKeyRoot, LPCSTR lpSubKey, LPCSTR lpValue, void* pData, int* pnSize);
	bool RegSetBinaryValue(HKEY hKeyRoot, LPCSTR lpSubKey, LPCSTR lpValue, const void* pData, int* pnSize);
	bool RegGetStringValue(HKEY hKeyRoot, LPCSTR lpSubKey, LPCSTR lpValue, LPSTR pData, DWORD dwSize);
	bool RegSetStringValue(HKEY hKeyRoot, LPCSTR lpSubKey, LPCSTR lpValue, LPCSTR pData);
#else
    void FinishROMConfig(void);
#endif
    void EditROMConfig(void);

	// Preferences
	void LoadPreferences();
	void SavePreferences(bool saveAll);

	PaletteType m_PaletteType;

	char m_PrefsFile[_MAX_PATH];
	Preferences m_Preferences;

	bool m_WriteInstructionCounts;
};

#endif
