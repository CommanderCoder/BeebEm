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
/* Win32 port - Mike Wyatt 7/6/97 */
/* Conveted Win32 port to use DirectSound - Mike Wyatt 11/1/98 */
// 14/04/01 - Proved that I AM better than DirectSound, by fixing the code thereof ;P

#include "beebsound.h"

#include <math.h>

#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "6502core.h"
#include "port.h"
#include "beebmem.h"
#include "beebwin.h"
#include "common.h"
#include "audev.h"
#include "uefstate.h"
#include "main.h"
#include "speech.h"


//  #define DEBUGSOUNDTOFILE

#define PREFSAMPLERATE 44100
#define MAXBUFSIZE 1024

static unsigned char SoundBuf[MAXBUFSIZE];
unsigned char SpeechBuf[MAXBUFSIZE];

struct SoundSample
{
	const char *pFilename;
	unsigned char *pBuf;
	int len;
	int pos;
	bool playing;
	bool repeat;
};
static SoundSample SoundSamples[] = {
	{ "RelayOn.snd", NULL, 0, 0, false, false },
	{ "RelayOff.snd", NULL, 0, 0, false, false },
	{ "DriveMotor.snd", NULL, 0, 0, false, false },
	{ "HeadLoad.snd", NULL, 0, 0, false, false },
	{ "HeadUnload.snd", NULL, 0, 0, false, false },
	{ "HeadSeek.snd", NULL, 0, 0, false, false },
	{ "HeadStep.snd", NULL, 0, 0, false, false }
};
#define NUM_SOUND_SAMPLES (sizeof(SoundSamples)/sizeof(SoundSample))
static bool SoundSamplesLoaded = false;

int SoundEnabled = 1;
int RelaySoundEnabled = 0;
int DiscDriveSoundEnabled = 0;
int SoundChipEnabled = 1;
int SoundSampleRate = PREFSAMPLERATE;
int SoundVolume = 3;
int SoundAutoTriggerTime;
int SoundBufferSize,TotalBufferSize;
double CSC[4]={0,0,0,0},CSA[4]={0,0,0,0}; // ChangeSamps Adjusts
char SoundExponentialVolume = 1;

/* Number of places to shift the volume */
#define VOLMAG 3

int Speech[3];

FILE *sndlog=NULL;

volatile struct BeebState76489_S {
	unsigned int ToneFreq[4];
	unsigned int ChangeSamps[4]; /* How often this channel should flip its otuput */
	unsigned int ToneVolume[4]; /* In units of /dev/dsp */
	struct {
		unsigned int FB:1; /* =0 for periodic, =1 for white */
		unsigned int Freq:2; /* 0=low, 1=medium, 2=high, 3=tone gen 1 freq */
		unsigned int Vol:4;
	} Noise;
	int LastToneFreqSet; /* the tone generator last set - for writing the 2nd byte */
} BeebState76489;

int RealVolumes[4]; // Holds the real volume values for state save use

static int ActiveChannel[4]={FALSE,FALSE,FALSE,FALSE}; /* Those channels with non-0 voolume */
// Set it to an array for more accurate sound generation
static unsigned int samplerate;
static double OurTime=0.0; /* Time in sample periods */

int SoundTrigger; /* Time to trigger a sound event */

static unsigned int GenIndex[4]; /* Used by the voice generators */
static int GenState[4];
static int bufptr=0;
int SoundDefault;
double SoundTuning=0.0; // Tunning offset

void PlayUpTil(double DestTime);
int GetVol(int vol);
bool bReRead=false;
volatile bool bDoSound=true;
int WriteOffset=0;
// LARGE_INTEGER PFreq, LastPCount, CurrentPCount;
double CycleRatio;
struct AudioType TapeAudio;
bool TapeSoundEnabled;
int PartSamples=1;
int SBSize=1;
bool Playing = 0;

/****************************************************************************/
/* DestTime is in samples */
void PlayUpTil(double DestTime) {
	int tmptotal,channel,bufinc,tapetotal;
	char Extras;
	int SpeechPtr = 0;
	int i;
	
//		fprintf(stderr, "Sound Samples = %d\n", (int) (DestTime - OurTime));
	if (MachineType != 3 && SpeechEnabled)
	{
		SpeechPtr = 0;
		memset(SpeechBuf, 128, sizeof(SpeechBuf));
		int len = (int) (DestTime - OurTime + 1);
		if (len > MAXBUFSIZE)
		{
			WriteLog("Speech Buffer Overflow, len = %d\n", len);
			len = MAXBUFSIZE;
		}
		tms5220_update(SpeechBuf, len);
	}

//	if (OurTime > DestTime)
//		WriteLog("OurTime = %g, DestTime = %g\n", OurTime, DestTime);
	
	while (DestTime>OurTime) {
			
		for(bufinc=0;(bufptr<SoundBufferSize) && ((OurTime+bufinc)<DestTime);bufptr++,bufinc++) {
			int tt;
			tmptotal=0;
			Extras=4;

			if (SoundChipEnabled) {
				// Begin of for loop
				for(channel=1;channel<=3;channel++) {
					if (ActiveChannel[channel]) {
						if ((GenState[channel]) && (!Speech[channel]))
							tmptotal+=BeebState76489.ToneVolume[channel];
						if ((!GenState[channel]) && (!Speech[channel]))
							tmptotal-=BeebState76489.ToneVolume[channel];
						if (Speech[channel])
							tmptotal+=(BeebState76489.ToneVolume[channel]-GetVol(7));
						GenIndex[channel]++;
						tt=(int)CSC[channel];
						if (!PartSamples) tt=0;
						if (GenIndex[channel]>=(BeebState76489.ChangeSamps[channel]+tt)) {
							if (CSC[channel] >= 1.0)
								CSC[channel]-=1.0;
							CSC[channel]+=CSA[channel];
							GenIndex[channel]=0;
							GenState[channel]^=1;
						}
					}
				} /* Channel loop */

				/* Now put in noise generator stuff */
				if (ActiveChannel[0]) { 
					if (BeebState76489.Noise.FB) {
						/* White noise */
						if (GenState[0])
							tmptotal+=BeebState76489.ToneVolume[0];
						else
							tmptotal-=BeebState76489.ToneVolume[0];
						GenIndex[0]++;
						switch (BeebState76489.Noise.Freq) {
						case 0: /* Low */
							if (GenIndex[0]>=(samplerate/10000)) {
								GenIndex[0]=0;
								GenState[0]=rand() & 1;
							}
							break;

						case 1: /* Med */
							if (GenIndex[0]>=(samplerate/5000)) {
								GenIndex[0]=0;
								GenState[0]=rand() & 1;
							}
							break;

						case 2: /* High */
							if (GenIndex[0]>=(samplerate/2500)) {
								GenIndex[0]=0;
								GenState[0]=rand() & 1;
							}
							break;

						case 3: /* as channel 1 */
							if (GenIndex[0]>=BeebState76489.ChangeSamps[1]) {
								GenIndex[0]=0;
								GenState[0]=rand() & 1;
							}
							break;
						} /* Freq type switch */
					} else {
						/* Periodic */
						if (GenState[0])
							tmptotal+=BeebState76489.ToneVolume[0];
						else
							tmptotal-=BeebState76489.ToneVolume[0];
						GenIndex[0]++;
						switch (BeebState76489.Noise.Freq) {
						case 2: /* Low */
							if (GenState[0]) {
								if (GenIndex[0]>=(samplerate/125)) {
									GenIndex[0]=0;
									GenState[0]=0;
								}
							} else {
								if (GenIndex[0]>=(samplerate/1250)) {
									GenIndex[0]=0;
									GenState[0]=1;
								}
							}
							break;

						case 1: /* Med */
							if (GenState[0]) {
								if (GenIndex[0]>=(samplerate/250)) {
									GenIndex[0]=0;
									GenState[0]=0;
								}
							} else {
								if (GenIndex[0]>=(samplerate/2500)) {
									GenIndex[0]=0;
									GenState[0]=1;
								}
							}
							break;

						case 0: /* High */
							if (GenState[0]) {
								if (GenIndex[0]>=(samplerate/500)) {
									GenIndex[0]=0;
									GenState[0]=0;
								}
							} else {
								if (GenIndex[0]>=(samplerate/5000)) {
									GenIndex[0]=0;
									GenState[0]=1;
								}
							}
							break;

						case 3: /* Tone gen 1 */
								
							tt = (int) CSC[0];
							if (GenIndex[0]>=(BeebState76489.ChangeSamps[1]+tt)) {
								static int per=0;
								CSC[0]+=CSA[1]-tt;
								GenIndex[0]=0;
								GenState[0]=(per==0);
								if (++per==30) per=0;
							}
								
							break;
								
						} /* Freq type switch */
					}
				}
			}

			// Mix in speech sound
 
			if (SpeechEnabled) if (MachineType != 3) tmptotal += (SpeechBuf[SpeechPtr++]-128)*10;

			// Mix in sound samples here
			for (i = 0; i < NUM_SOUND_SAMPLES; ++i) {
				if (SoundSamples[i].playing) {
					tmptotal+=(SoundSamples[i].pBuf[SoundSamples[i].pos]-128)*10;
					Extras++;
					SoundSamples[i].pos += (44100 / samplerate);
					if (SoundSamples[i].pos >= SoundSamples[i].len) {
						if (SoundSamples[i].repeat)
							SoundSamples[i].pos = 0;
						else
							SoundSamples[i].playing = false;
					}
				}
			}
			
			if (TapeSoundEnabled) {
				// Mix in tape sound here
				tapetotal=0; 
				if ((TapeAudio.Enabled) && (TapeAudio.Signal==2)) {
					if (TapeAudio.Samples++>=36) TapeAudio.Samples=0;
					tapetotal=(int)(sin(((TapeAudio.Samples*20)*3.14)/180)*80);
					Extras++;
				}
				if ((TapeAudio.Enabled) && (TapeAudio.Signal==1)) {
					tapetotal=(int)(sin(((TapeAudio.Samples*(10*(1+TapeAudio.CurrentBit)))*3.14)/180)*(80+(40*(1-TapeAudio.CurrentBit))));
					// And if you can follow that equation, "ill give you the money meself" - Richard Gellman
					if (TapeAudio.Samples++>=36) {
						TapeAudio.Samples=0;
						TapeAudio.BytePos++;
						if (TapeAudio.BytePos<=10) TapeAudio.CurrentBit=(TapeAudio.Data & (1<<(10-TapeAudio.BytePos)))?1:0;
					}
					if (TapeAudio.BytePos>10) {
						TapeAudio.ByteCount--;
						if (!TapeAudio.ByteCount) TapeAudio.Signal=2; else { TapeAudio.BytePos=1; TapeAudio.CurrentBit=0; }
					}
					Extras++;
				}
				tmptotal+=tapetotal;
			}

			/* Make it a bit louder under Windows */
			if (Extras) 
				tmptotal/=Extras; 
			else 
				tmptotal=0;

			SoundBuf[bufptr] = (tmptotal/SoundVolume)+128;
			// end of for loop
		} /* buffer loop */

//     fprintf(stderr,"PlayUpTil: bufptr = %d\n", bufptr);

	  if (SoundEnabled)
	  {

	    audev_play_buff(SoundBuf, bufptr);

		if (mainWin->m_pMovie)
		{
			memcpy(mainWin->m_soundBufferPtr + mainWin->m_soundBufferLen, SoundBuf, bufptr);
			mainWin->m_soundBufferLen += bufptr; 
		}
	  
	  }
 /*   fprintf(stderr,"PlayUpTil: After write: bufptr=%d OurTime=%f\n",bufptr,OurTime);*/
      OurTime+=bufinc;
      bufptr=0;

	} /* While time */ 
}; /* PlayUpTil */

/****************************************************************************/
/* Convert time in cycles to time in samples                                */
static int LastBeebCycle=0; /* Last parameter to this function */
static double LastOurTime=0; /* Last result of this function */

static double CyclesToSamples(int BeebCycles) {
  double tmp;

  /* OK - beeb cycles are in 2MHz units, ours are in 1/samplerate */
  /* This is all done incrementally - find the number of ticks since the last call
     in both domains.  This does mean this should only be called once */
  /* Extract number of cycles since last call */
  if (BeebCycles<LastBeebCycle) {
    /* Wrap around in beebs time */
    tmp=((double)CycleCountWrap-(double)LastBeebCycle)+(double)BeebCycles;
  } else {
    tmp=(double)BeebCycles-(double)LastBeebCycle;
  };

//  tmp/=(mainWin->m_RealTimeTarget)?mainWin->m_RealTimeTarget:1;

/*fprintf(stderr,"Convert tmp=%f\n",tmp); */
  LastBeebCycle=BeebCycles;

  tmp*=(samplerate);
  tmp/=2000000.0; /* Few - glad thats a double! */

  LastOurTime+=tmp;
  return LastOurTime;
}; /* CyclesToSamples */

/****************************************************************************/
static void InitAudioDev(int sampleratein) {

  extraopt_t dummyopt = {NULL, NULL};
  int res;
  
  samplerate=sampleratein;

  res = audev_init_device(NULL, PREFSAMPLERATE, 1, &dummyopt);
  if (res == FALSE) {
    fprintf(stderr, "Couldn't open audio device\n");
    SoundEnabled = 0;
	SoundChipEnabled = 0;
  }
  else
    SoundEnabled = 1;

  
}; /* InitAudioDev */

void LoadSoundSamples(void) {
	FILE *fd;
	char FileName[256];
	int i;
	
	if (!SoundSamplesLoaded) {
		for (i = 0; i < NUM_SOUND_SAMPLES; ++i) {
			strcpy(FileName, RomPath);
			strcat(FileName, SoundSamples[i].pFilename);
			fd = fopen(FileName, "rb");
			if (fd != NULL) {
				fseek(fd, 0, SEEK_END);
				SoundSamples[i].len = ftell(fd);
				SoundSamples[i].pBuf = (unsigned char *)malloc(SoundSamples[i].len);
				fseek(fd, 0, SEEK_SET);
				fread(SoundSamples[i].pBuf, 1, SoundSamples[i].len, fd);
				fclose(fd);
			}
			else {
				fprintf(stderr, "Could not open sound sample file: %s\n", FileName);
			}
		}
		SoundSamplesLoaded = true;
	}
}

/****************************************************************************/
/* The 'freqval' variable is the value as sene by the 76489                 */
static void SetFreq(int Channel, int freqval) {
  unsigned int freq;
  int ChangeSamps; /* Number of samples after which to change */
  //fprintf(sndlog,"Channel %d - Value %d\n",Channel,freqval);
  double t;

  if (freqval==0) freqval=1;
  if (freqval<5) Speech[Channel]=1; else Speech[Channel]=0;
  freq=4000000/(32*freqval);

  t=( (( (double)samplerate/(double)freq)/2.0) +SoundTuning);
  ChangeSamps=(int)t;
  CSA[Channel]=(double)(t-ChangeSamps);
	CSC[Channel]=CSA[Channel];  // we look ahead, so should already include the fraction on the first update
	if (Channel==1) {
		CSC[0]=CSC[1];
	}
	BeebState76489.ChangeSamps[Channel]=ChangeSamps;
};

void SoundTrigger_Real(void) {
  double nowsamps;
  
  nowsamps = CyclesToSamples(TotalCycles);
  PlayUpTil(nowsamps);

  SoundTrigger=TotalCycles+SoundAutoTriggerTime;

//	WriteLog(" After : TotalCycles = %d, XtraCycles = %d, SoundTrigger = %d\n", 
//			 TotalCycles, XtraCycles, SoundTrigger, SoundCycles);

}; /* SoundTrigger_Real */

void Sound_Trigger(int NCycles) {

	if (SoundTrigger<=TotalCycles) SoundTrigger_Real();
	
	// fprintf(sndlog,"SoundTrigger_Real was called from Sound_Trigger\n"); }
}

void SoundChipReset(void) {
  BeebState76489.LastToneFreqSet=0;
  BeebState76489.ToneVolume[0]=0;
  BeebState76489.ToneVolume[1]=BeebState76489.ToneVolume[2]=BeebState76489.ToneVolume[3]=GetVol(15);
  BeebState76489.ToneFreq[0]=BeebState76489.ToneFreq[1]=BeebState76489.ToneFreq[2]=1000;
  BeebState76489.ToneFreq[3]=1000;
  BeebState76489.Noise.FB=0;
  BeebState76489.Noise.Freq=0;
  ActiveChannel[0]=FALSE;
  ActiveChannel[1]=ActiveChannel[2]=ActiveChannel[3]=FALSE;
}

/****************************************************************************/
/* Called to enable sound output                                            */
void SoundInit() {
  ClearTrigger(SoundTrigger);
  LastBeebCycle=TotalCycles;
  LastOurTime=(double)LastBeebCycle * (double)SoundSampleRate / 2000000.0;
  OurTime=LastOurTime;
  bufptr=0;
  InitAudioDev(SoundSampleRate);
  if (SoundSampleRate == 44100) SoundAutoTriggerTime = 5000; 
  if (SoundSampleRate == 22050) SoundAutoTriggerTime = 10000; 
  if (SoundSampleRate == 11025) SoundAutoTriggerTime = 20000; 
//  SampleAdjust=4;
  SoundBufferSize=SoundSampleRate/50;
  LoadSoundSamples();
  bReRead=true;
  SoundTrigger=TotalCycles+SoundAutoTriggerTime;
}; /* SoundInit */

void SwitchOnSound(void) {
  SetFreq(3,1000);
  ActiveChannel[3]=TRUE;
  BeebState76489.ToneVolume[3]=GetVol(15);
}

void SetSound(char State) {
	if (!SoundEnabled) return;
}


/****************************************************************************/
/* Called to disable sound output                                           */
void SoundReset(void) {

  if (SoundEnabled)
    audev_close_device();
  ClearTrigger(SoundTrigger);
  SoundEnabled = 0;
} /* SoundReset */

/****************************************************************************/
/* Called in sysvia.cc when a write is made to the 76489 sound chip         */
void Sound_RegWrite(int value) {
  int trigger = 0;
  unsigned char VolChange;

  if (!SoundEnabled)
    return;
  VolChange=4;

  if (!(value & 0x80)) {
    unsigned val=BeebState76489.ToneFreq[BeebState76489.LastToneFreqSet] & 15;

    /* Its changing the top half of the frequency */
    val |= (value & 0x3f)<<4;

    /* And update */
    BeebState76489.ToneFreq[BeebState76489.LastToneFreqSet]=val;
    SetFreq(BeebState76489.LastToneFreqSet+1,BeebState76489.ToneFreq[BeebState76489.LastToneFreqSet]);
    trigger = 1;
  } else {
    /* Another register */
	VolChange=0xff;
    switch ((value>>4) & 0x7) {
      case 0: /* Tone 3 freq */
        BeebState76489.ToneFreq[2]=(BeebState76489.ToneFreq[2] & 0x3f0) | (value & 0xf);
        SetFreq(3,BeebState76489.ToneFreq[2]);
        BeebState76489.LastToneFreqSet=2;
//		trigger = 1;
        break;

      case 1: /* Tone 3 vol */
        RealVolumes[3]=value&15;
		if ((BeebState76489.ToneVolume[3]==0) && ((value &15)!=15)) ActiveChannel[3]=TRUE;
        if ((BeebState76489.ToneVolume[3]!=0) && ((value &15)==15)) ActiveChannel[3]=FALSE;
        BeebState76489.ToneVolume[3]=GetVol(15-(value & 15));
        BeebState76489.LastToneFreqSet=2;
		trigger = 1;
		VolChange=3;
        break;

      case 2: /* Tone 2 freq */
        BeebState76489.ToneFreq[1]=(BeebState76489.ToneFreq[1] & 0x3f0) | (value & 0xf);
        BeebState76489.LastToneFreqSet=1;
        SetFreq(2,BeebState76489.ToneFreq[1]);
//		trigger = 1;
        break;

      case 3: /* Tone 2 vol */
        RealVolumes[2]=value&15;
        if ((BeebState76489.ToneVolume[2]==0) && ((value &15)!=15)) ActiveChannel[2]=TRUE;
        if ((BeebState76489.ToneVolume[2]!=0) && ((value &15)==15)) ActiveChannel[2]=FALSE;
        BeebState76489.ToneVolume[2]=GetVol(15-(value & 15));
        BeebState76489.LastToneFreqSet=1;
		trigger = 1;
		VolChange=2;
        break;

      case 4: /* Tone 1 freq (Possibly also noise!) */
        BeebState76489.ToneFreq[0]=(BeebState76489.ToneFreq[0] & 0x3f0) | (value & 0xf);
        BeebState76489.LastToneFreqSet=0;
        SetFreq(1,BeebState76489.ToneFreq[0]);
//		trigger = 1;
        break;

      case 5: /* Tone 1 vol */
        RealVolumes[1]=value&15;
        if ((BeebState76489.ToneVolume[1]==0) && ((value &15)!=15)) ActiveChannel[1]=TRUE;
        if ((BeebState76489.ToneVolume[1]!=0) && ((value &15)==15)) ActiveChannel[1]=FALSE;
        BeebState76489.ToneVolume[1]=GetVol(15-(value & 15));
        BeebState76489.LastToneFreqSet=0;
		trigger = 1;
		VolChange=1;
        break;

      case 6: /* Noise control */
        BeebState76489.Noise.Freq=value &3;
        BeebState76489.Noise.FB=(value>>2)&1;

        trigger = 1;
        break;

      case 7: /* Noise volume */
        if ((BeebState76489.ToneVolume[0]==0) && ((value &15)!=15)) ActiveChannel[0]=TRUE;
        if ((BeebState76489.ToneVolume[0]!=0) && ((value &15)==15)) ActiveChannel[0]=FALSE;
		RealVolumes[0]=value&15;
        BeebState76489.ToneVolume[0]=GetVol(15-(value & 15));
        trigger = 1;
		VolChange=0;
        break;
    };
   //if (VolChange<4) fprintf(sndlog,"Channel %d - Volume %d at %lu Cycles\n",VolChange,value &15,SoundCycles);
  };

  if (trigger) 
    SoundTrigger_Real();
  
}; /* Sound_RegWrite */

void DumpSound(void) {
}

void ClickRelay(unsigned char RState) {
	if (RelaySoundEnabled) {
		if (RState) {
			SoundSamples[SAMPLE_RELAY_ON].pos = 0;
			SoundSamples[SAMPLE_RELAY_ON].playing = true;
		}
		else {
			SoundSamples[SAMPLE_RELAY_OFF].pos = 0;
			SoundSamples[SAMPLE_RELAY_OFF].playing = true;
		}
	}
}

void PlaySoundSample(int sample, bool repeat) {
	SoundSamples[sample].pos = 0;
	SoundSamples[sample].playing = true;
	SoundSamples[sample].repeat = repeat;
}

void StopSoundSample(int sample) {
	SoundSamples[sample].playing = false;
}

int GetVol(int vol) {
	if (SoundExponentialVolume) {
//		static int expVol[] = {0,  2,  4,  6,  9, 12, 15, 19, 24, 30, 38, 48, 60, 76,  95, 120 } ;
		static int expVol[] = {0, 11, 14, 17, 20, 24, 28, 33, 39, 46, 54, 63, 74, 87, 102, 120 } ;
		if (vol >= 0 && vol <= 15)
			return expVol[vol];
		else
			return 0;
	}
	else {
		return vol << VOLMAG;
	}
}

void LoadSoundUEF(FILE *SUEF) {
	// Sound block
	unsigned char Chan;
	int Data;
	int RegVal; // This will be filled in by the data processor
	for (Chan=1;Chan<4;Chan++) {
		Data=fget16(SUEF);
		// Send the data direct to Sound_RegWrite()
		RegVal=(((Chan-1)*2)<<4)|128;
		RegVal|=(Data&15);
		Sound_RegWrite(RegVal);
		RegVal=(Data&1008)>>4;
		Sound_RegWrite(RegVal);
	}
	for (Chan=1;Chan<4;Chan++) {
		Data=fgetc(SUEF);
		RegVal=((((Chan-1)*2)+1)<<4)|128;
		RegVal|=Data&15;
		Sound_RegWrite(RegVal);
		BeebState76489.ToneVolume[4-Chan]=GetVol(15-Data);
		if (Data!=15) ActiveChannel[4-Chan]=TRUE; else ActiveChannel[4-Chan]=FALSE;
	}
	RegVal=224|(fgetc(SUEF)&7);
	Sound_RegWrite(RegVal);
	Data=fgetc(SUEF);
	RegVal=240|(Data&15);
	Sound_RegWrite(RegVal);
	BeebState76489.ToneVolume[0]=GetVol(15-Data);
	if (Data!=15) ActiveChannel[0]=TRUE; else ActiveChannel[0]=FALSE;
	BeebState76489.LastToneFreqSet=fgetc(SUEF);
	GenIndex[0]=fget16(SUEF);
	GenIndex[1]=fget16(SUEF);
	GenIndex[2]=fget16(SUEF);
	GenIndex[3]=fget16(SUEF);
}

void SaveSoundUEF(FILE *SUEF) {
	unsigned char Noise;
	fput16(0x046B,SUEF);
	fput32(20,SUEF);
	// Sound Block
	fput16(BeebState76489.ToneFreq[2],SUEF);
	fput16(BeebState76489.ToneFreq[1],SUEF);
	fput16(BeebState76489.ToneFreq[0],SUEF);
	fputc(RealVolumes[3],SUEF);
	fputc(RealVolumes[2],SUEF);
	fputc(RealVolumes[1],SUEF);
    Noise=BeebState76489.Noise.Freq |
          (BeebState76489.Noise.FB<<2);
	fputc(Noise,SUEF);
	fputc(RealVolumes[0],SUEF);
	fputc(BeebState76489.LastToneFreqSet,SUEF);
	fput16(GenIndex[0],SUEF);
	fput16(GenIndex[1],SUEF);
	fput16(GenIndex[2],SUEF);
	fput16(GenIndex[3],SUEF);
}
