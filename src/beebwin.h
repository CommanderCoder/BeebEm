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
#ifndef BEEBWIN_HEADER
#define BEEBWIN_HEADER

#include <Carbon/Carbon.h>
#include <string.h>
#if 0 //ACH - quicktime
#include <QuickTime/QuickTime.h>
#include <QuickTime/QTML.h>
#endif

#include "port.h"
#include "video.h"
#include "disctype.h"

#include "model.h"

extern Model MachineType;

typedef union {
	unsigned char data[8];
  EightByteType eightbyte;
} EightUChars;

typedef union {
	unsigned char data[16];
  EightByteType eightbytes[2];
} SixteenUChars;

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

extern int transTable1[256][2];

class BeebWin {

	int OldAutoRepeat; /* -1 means we don't know, 0 and 1 are as returned from XGetKeyboardControl */
	int DataSize;

public:
	unsigned char cols[256]; /* Beeb colour lookup */
	char *m_screen;
	char *m_screen_blur;
	WindowRef mWindow; 
	PixMap mBitMap;
	CTabPtr mCT;
	int_fast32_t m_RGB32[256];
	int_fast16_t m_RGB16[256];
	char m_clipboard[32768];
	int m_clipboardlen;
	int	m_clipboardptr;
	char m_printerbuffer[1024 * 1024];
	int m_printerbufferlen;
	
	enum PaletteType { RGB, BW, AMBER, GREEN } palette_type;

	void SetupClipboard(void);
	void ResetClipboard(void);
	int m_OSRDCH;
	void doCopy(void);
	void doPaste(void);
	void CopyKey(int data);
	int PasteKey(int addr);
	int KeyDown(int vkey);
	int KeyUp(int vkey);
	int TranslateKey(int vkey, int keyUp, int &row, int &col);

	void Initialise(char *home);

	BeebWin();
	~BeebWin();
	void updateLines(int starty, int nlines);
	void screenblit(int starty, int nlines);
	void bufferblit(int starty, int nlines);
	void bufferblit1(int starty, int nlines);
	void bufferblit2(int starty, int nlines);
	void DumpScreen(int offset);

	void doHorizLine(int  Colour, int y, int sx, int width);
	void doInvHorizLine(int Colour, int y, int sx, int width);
	void doUHorizLine(int Colour, int y, int sx, int width);
	void doHorizLine1(unsigned long Col, int y, int sx, int width);
	void doInvHorizLine1(unsigned long Col, int y, int sx, int width);
	void doUHorizLine1(unsigned long Col, int offset, int width);
	EightUChars *GetLinePtr(int y);
	SixteenUChars *GetLinePtr16(int y);

	void UpdateEconetMenu();
	void UpdateAMXMenu();
	void UpdateModelType();
	void SetSoundMenu(void);
	void SetPBuff(void);
	void SetImageName(const char *DiscName,int Drive,DiscType DType);
	void SetTapeSpeedMenu(void);
	void SetDiscWriteProtects(void);
	void SetRomMenu(void);				// LRW  Added for individual ROM/Ram
	void SelectFDC(void);
	void LoadFDC(char *DLLName, bool save);
	void KillDLLs(void);
	void UpdateLEDMenu(void);
	void SetDriveControl(unsigned char value);
	unsigned char GetDriveControl(void);
	void doLED(int sx,bool on);

	void RealizePalette(void) {};
	void ResetBeebSystem(Model NewModelType,unsigned char TubeStatus,unsigned char LoadRoms);
    
    void CreateDiscImage(const char *Filename, int Drive, int Heads, int Tracks);

	void TranslateFDC(void);
	void TranslateTiming(int TimingId);
	int StartOfFrame(void);
	bool UpdateTiming(void);
	void AdjustSpeed(bool up);
	void DisplayTiming(void);
	unsigned long GetTickCount(void);
	void ScaleJoystick(unsigned int x, unsigned int y);
	void SetMousestickButton(int index, bool button);
	void ScaleMousestick(unsigned int x, unsigned int y);
	void HandleCommand(int MenuId);
	void SetAMXPosition(unsigned int x, unsigned int y);
	void Focus(bool);

	bool IsFrozen(void);
	void ShowMenu(bool on);
	void TrackPopupMenu(int x, int y);
	bool IsFullScreen() { return m_isFullScreen; }
	void SaveOnExit(void);
	void ResetTiming(void);
	void HandleCommandLine(char *cmd);
	void NewTapeImage(char *FileName);
	void ReadDisc(int drive);
	void LoadDisc(int drive, char *path);
    void MReadRom(int rom);
    void MLoadRom(int rom, char *path);
    void MClearRom(int rom);
	void LoadTapeFromPath(char *path);
	OSStatus HandleCommand(UInt32 cmdID);
	void SetMenuCommandIDCheck(UInt32 commandID, bool check);

	bool		m_frozen;
	double		m_RealTimeTarget;
	int			m_ShiftBooted;
	Ptr			m_RestoreState;
	bool		m_isFullScreen;
	bool		m_maintainAspectRatio;
	bool		m_PrintScreen;
	bool		m_PrintToPDF;
	bool		m_CopyToClip;
	bool		m_Invert;
	int			m_PicNum;
	int			m_MenuIdPrinterPort;
	int			m_XWinSize;
	int			m_YWinSize;
	bool		m_FreezeWhenInactive;

  private:
	int			m_MenuIdWinSize;
	int			m_XWinPos;
	int			m_YWinPos;
	bool		m_FullScreen;
	WindowPtr	m_FullScreenWindow;
	bool		m_ShowSpeedAndFPS;
	int			m_MenuIdSampleRate;
	int			m_MenuIdVolume;
	int			m_DiscTypeSelection;
	int			m_MenuIdTiming;
	int			m_FPSTarget;
//	JOYCAPS		m_JoystickCaps;
	int			m_MenuIdSticks;
	bool		m_HideCursor;
	int			m_MenuIdKeyMapping;
	int			m_KeyMapAS;
	int			m_KeyMapFunc;
	int			m_ShiftPressed;
	int			m_vkeyPressed[256][2][2];
	char		m_AppPath[256];
	bool		m_WriteProtectDisc[2];
	bool		m_WriteProtectOnLoad;
	int			m_MenuIdAMXSize;
	int			m_MenuIdAMXAdjust;
	int			m_AMXXSize;
	int			m_AMXYSize;
	int			m_AMXAdjust;
	
	char		m_szTitle[100];

	int			m_ScreenRefreshCount;
	double		m_RelativeSpeed;
	double		m_FramesPerSecond;

	char		m_PrinterFileName[256];
	char		m_PrinterDevice[256];

	long		m_LastTickCount;
	long		m_LastStatsTickCount;
	long		m_LastTotalCycles;
	long		m_LastStatsTotalCycles;
	long		m_TickBase;
	long		m_CycleBase;
	long		m_MinFrameCount;
	long		m_LastFPSCount;
	int			m_Motion_Blur;
	char		m_BlurIntensities[8];
	
	bool InitClass(void);
	void UpdateOptiMenu(void);
	void CreateBeebWindow(void);
	void CreateBitmap(void);
	void InitMenu(void);
	void UpdatePalette(PaletteType NewPalete);
	void UpdateMonitorMenu();
	void UpdateMotionBlurMenu();
	void UpdateSerialMenu();
	void UpdateSFXMenu();
	void GetRomMenu(void);				// LRW  Added for individual ROM/Ram
	void TranslateWindowSize(int size);
	void TranslateSampleRate(void);
	void TranslateVolume(void);
	void TranslateKeyMapping(void);
	void TranslateCapture(void);
	void LoadTape(void);
	void InitJoystick(void);
	void ResetJoystick(void);
	void RestoreState(void);
	void SaveState(void);
	void NewDiscImage(int Drive);
	void EjectDiscImage(int Drive);
	void ToggleWriteProtect(int Drive);
	void LoadPreferences(void);
	void SavePreferences(void);
	void SetWindowAttributes(bool wasFullScreen);
	void TranslateAMX(void);
	bool PrinterFile(void);
	void TogglePrinter(void);
	void TranslatePrinterPort(void);
	void SaveWindowPos(void);
	void LoadUserKeyMap (void);
	void SaveUserKeyMap (void);
	void ImportDiscFiles(int menuId);
	void ExportDiscFiles(int menuId);
    void SaveCMOS(void);

#if 0 //ACH - capture video
public:
		
	void CaptureVideo();
	void StartRecordingVideo(char *path, FSSpec *fs);
	void EndCaptureVideo();

	Track	m_pTrack;
	Media	m_pMedia;
	Movie	m_pMovie;
	short	m_resRefNum;
	Handle	m_compressedData;
	Ptr		m_compressedDataPtr;
	ImageDescriptionHandle m_imageDesc;
	SoundDescriptionHandle m_soundDesc;
	Track	m_sTrack;
	Media	m_sMedia;
	Rect	m_trackFrame;
	Handle	m_soundBuffer;
	Ptr		m_soundBufferPtr;
	int		m_soundBufferLen;
	ComponentInstance	m_ci;
	bool	m_firstFrame;
	long	m_keyFrame;
	long	m_keyFrameCount;
	long	m_frameSkip;
	long	m_frameSkipCount;
	long	m_frameDuration;
	long	m_frameSize;
#endif
	int	m_skip;
	int	m_captureresolution;
	
}; /* BeebWin */

void SaveEmuUEF(FILE *SUEF);
void LoadEmuUEF(FILE *SUEF, int Version);
#endif
