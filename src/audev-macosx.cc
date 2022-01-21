/* Boodler: a programmable soundscape tool
   Copyright 2001 by Andrew Plotkin <erkyrath@eblong.com>
   <http://www.eblong.com/zarf/boodler/>
   This program is distributed under the LGPL.
   See the LGPL document, or the above URL, for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <CoreAudio/CoreAudio.h>
/* "/System/Library/Frameworks/CoreAudio.framework/Versions/A/Headers/CoreAudio.h" */

#include "common.h"
#include "audev.h"

#include "main.h"

typedef struct buffer_struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int full;
    float *buf;
} buffer_t;

static AudioDeviceID audevice = kAudioDeviceUnknown;
static long sound_rate = 0; /* frames per second */
static int sound_channels = 0;
static long sound_buffersize = 0; /* bytes */

static int bufcount = 8;
static long samplesperbuf = 0;
static long framesperbuf = 0;

static int bailing;

static int filling, emptying;
static buffer_t *rawbuffer;
static long *valbuffer = NULL; /* samplesperbuf longs */

static OSStatus PlaybackIOProc(AudioDeviceID inDevice,
        const AudioTimeStamp *inNow,
        const AudioBufferList *inInputData,
        const AudioTimeStamp *inInputTime,
        AudioBufferList *outOutputData,
        const AudioTimeStamp *inOutputTime,
        void *inClientData);

/*

   void SetNominalSampleRate(Float64 inSampleRate)
   {
   UInt32 theSize = sizeof(Float64);
   SetPropertyData(0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, theSize, &inSampleRate);
   }

 */

int audev_init_device(char *dummydevname, long ratewanted, int verbose, extraopt_t *extra)
{
    int bx, res;
    OSStatus status;
    int channels;
    long rate;
    long fragsize;
    extraopt_t *opt;
    UInt32 propsize;
    UInt32 bytecount;
    struct AudioStreamBasicDescription streamdesc;

#define LEN_DEVICE_NAME 128
    char devicename[LEN_DEVICE_NAME];

    fragsize = 1024 * 8;
    bufcount = 8;

    for (opt=extra; opt->key; opt++) {
        if (!strcmp(opt->key, "buffersize") && opt->val) {
            fragsize = atol(opt->val);
        }
        else if (!strcmp(opt->key, "buffercount") && opt->val) {
            bufcount = atoi(opt->val);
        }
    }

    if (bufcount < 2)
        bufcount = 2;

    if (audevice != kAudioDeviceUnknown) {
        fprintf(stderr, "Sound device is already open.\n");
        return FALSE;
    }

    AudioObjectPropertyAddress theAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 mypropsize;
    status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &mypropsize);

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
            &theAddress, 
            0,
            NULL,
            &mypropsize,
            &audevice);

    if (status) {
        fprintf(stderr, "Could not get audio default device.\n");
        return FALSE;
    }

    if (audevice == kAudioDeviceUnknown) {
        fprintf(stderr, "Audio default device is unknown.\n");
        return FALSE;
    }

    
    UInt32 propertySize;
    char mydevicename[128];

        theAddress.mSelector = kAudioObjectPropertyName;
        theAddress.mScope = kAudioObjectPropertyScopeGlobal;
        theAddress.mElement = kAudioObjectPropertyElementMaster;
    
        status = AudioObjectGetPropertyDataSize(audevice, &theAddress, 0, NULL, &propertySize);
        
        status = AudioObjectGetPropertyData(audevice,
              &theAddress,
              0,
              NULL,
              &propertySize,
              mydevicename);
    

//    propsize = sizeof(devicename);
//    status = AudioDeviceGetProperty(audevice, 1, 0,
//            kAudioDevicePropertyDeviceName, &propsize, devicename);
    // When using the correct code above (i.e. not this depreciated code
    // we get the wrong default output device name - Don't know why

    if (status) {
        fprintf(stderr, "Could not get audio device name.\n");
        return FALSE;
    }

    if (verbose) {
        printf("Got device ID %d: \"%s\".\n", (int)audevice, devicename);
    }

    if (ratewanted) {

        UInt32 theSize = sizeof(Float64);
        Float64 inSampleRate = (Float64) ratewanted;

        theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
        theAddress.mScope = kAudioObjectPropertyScopeGlobal;
        theAddress.mElement = kAudioObjectPropertyElementMaster;

        //      status = AudioDeviceSetProperty(audevice,
        //              NULL,
        //              0,
        //              0,
        //              kAudioDevicePropertyNominalSampleRate, 
        //              theSize, 
        //              &inSampleRate);
        //

        status = AudioObjectSetPropertyData(audevice,
                &theAddress,
                0,
                NULL,
                theSize,
                &inSampleRate);

        if (status) {
            fprintf(stderr, "Could not set sample rate; continuing.\n");
        }
    }

    bytecount = (unsigned int)fragsize;
    propsize = sizeof(bytecount);

    //  status = AudioDeviceSetProperty(audevice, NULL, 0, 0,
    //          kAudioDevicePropertyBufferSize, propsize, &bytecount);

    theAddress.mSelector = kAudioDevicePropertyBufferSize;
    theAddress.mScope = kAudioObjectPropertyScopeGlobal;
    theAddress.mElement = kAudioObjectPropertyElementMaster;

    status = AudioObjectSetPropertyData(audevice,
            &theAddress,
            0,
            NULL,
            propsize,
            &bytecount);

    if (status) {
        fprintf(stderr, "Could not set buffer size; continuing.\n");
    }

    Float64 theAnswer = 0;
    UInt32 theSize = sizeof(Float64);

    //  status = AudioDeviceGetProperty(audevice, 1, 0,
    //          kAudioDevicePropertyNominalSampleRate, &theSize, &theAnswer);
    //
    theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    theAddress.mScope = kAudioObjectPropertyScopeGlobal;
    theAddress.mElement = kAudioObjectPropertyElementMaster;

    status = AudioObjectGetPropertyData(audevice,
            &theAddress,
            0,
            NULL,
            &theSize,
            &theAnswer);

    if (status) {
        fprintf(stderr, "Could not get nominal sample rate\n");
    }

    fprintf(stderr, "Nominal sample rate = %f\n", theAnswer);

    theSize = 0;

    //  AudioDeviceGetPropertyInfo(audevice, 1, 0, kAudioDevicePropertyAvailableNominalSampleRates, &theSize, NULL);

    theAddress.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
    theAddress.mScope = kAudioObjectPropertyScopeGlobal;
    theAddress.mElement = kAudioObjectPropertyElementMaster;

    status = AudioObjectGetPropertyData(audevice,
            &theAddress,
            0,
            NULL,
            &theSize,
            nullptr);


    fprintf(stderr, "There are %lu sample rates\n", theSize / sizeof(AudioValueRange));


    theSize = 10 * sizeof(AudioValueRange);
    AudioValueRange theRange[10];

    //  status = AudioDeviceGetProperty(audevice, 1, 0,
    //          kAudioDevicePropertyAvailableNominalSampleRates, &theSize, &theRange);

    theAddress.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
    theAddress.mScope = kAudioObjectPropertyScopeGlobal;
    theAddress.mElement = kAudioObjectPropertyElementMaster;

    status = AudioObjectGetPropertyData(audevice,
            &theAddress,
            0,
            NULL,
            &theSize,
            &theRange);

    if (status) {
        fprintf(stderr, "Could not get sample ratese\n");
    }

    fprintf(stderr, "There are %lu sample rates\n", theSize / sizeof(AudioValueRange));

    for (int i = 0; i < theSize / sizeof(AudioValueRange); ++i)
    {
        fprintf(stderr, "Min = %f, Max = %f\n", theRange[i].mMinimum, theRange[i].mMaximum);
    }


    propsize = sizeof(struct AudioStreamBasicDescription);
    //  status = AudioDeviceGetProperty(audevice, 1, 0,
    //          kAudioDevicePropertyStreamFormat, &propsize, &streamdesc);
    //
    theAddress.mSelector = kAudioDevicePropertyStreamFormat;
    theAddress.mScope = kAudioObjectPropertyScopeGlobal;
    theAddress.mElement = kAudioObjectPropertyElementMaster;

    status = AudioObjectGetPropertyData(audevice,
            &theAddress,
            0,
            NULL,
            &propsize,
            &streamdesc);

    if (status) {
        fprintf(stderr, "Could not get audio device description.\n");
        return FALSE;
    }

    fprintf(stderr, "mSampleRate = %f\n", streamdesc.mSampleRate);
    fprintf(stderr, "mFormatID = %u\n", (unsigned int)streamdesc.mFormatID);
    fprintf(stderr, "mFormatFlags = %u\n", (unsigned int)streamdesc.mFormatFlags);
    fprintf(stderr, "mBytesPerPacket = %u\n", (unsigned int)streamdesc.mBytesPerPacket);
    fprintf(stderr, "mFramesPerPacket = %u\n", (unsigned int)streamdesc.mFramesPerPacket);
    fprintf(stderr, "mBytesPerFrame = %u\n", (unsigned int)streamdesc.mBytesPerFrame);
    fprintf(stderr, "mChannelsPerFrame = %u\n", (unsigned int)streamdesc.mChannelsPerFrame);
    fprintf(stderr, "mBitsPerChannel = %u\n", (unsigned int)streamdesc.mBitsPerChannel);

    rate = (long) streamdesc.mSampleRate;

    if (streamdesc.mFormatID != kAudioFormatLinearPCM) {
        fprintf(stderr, "Audio device format is not LinearPCM; exiting.\n");
        return FALSE;    
    }

    if (streamdesc.mChannelsPerFrame != 2) {
        fprintf(stderr, "Audio device is not stereo; exiting.\n");
        return FALSE;
    }
    channels = 2;

    if (!(streamdesc.mFormatFlags & kLinearPCMFormatFlagIsFloat)) {
        fprintf(stderr, "Audio device is not floating-point; exiting.\n");
        return FALSE;
    }

    propsize = sizeof(bytecount);
    //  status = AudioDeviceGetProperty(audevice, 1, 0,
    //          kAudioDevicePropertyBufferSize, &propsize, &bytecount);
    //
    theAddress.mSelector = kAudioDevicePropertyBufferSize;
    theAddress.mScope = kAudioObjectPropertyScopeGlobal;
    theAddress.mElement = kAudioObjectPropertyElementMaster;

    status = AudioObjectGetPropertyData(audevice,
            &theAddress,
            0,
            NULL,
            &propsize,
            &bytecount);

    if (status) {
        fprintf(stderr, "Could not get audio device buffer size.\n");
        return FALSE;
    }

    fragsize = bytecount;
    if (verbose) {
        printf("%ld bytes per buffer.\n", fragsize);
    }

    if (verbose) {
        printf("%d buffers in queue.\n", bufcount);
    }

    /* Everything's figured out. */

    sound_rate = rate;
    sound_channels = channels;
    sound_buffersize = fragsize;

    framesperbuf = sound_buffersize / (sizeof(float) * sound_channels);
    samplesperbuf = framesperbuf * sound_channels;
    if (verbose) {
        printf("%ld frames (%ld samples) per buffer.\n",
                framesperbuf, samplesperbuf);
        printf("%ld frames per second.\n",
                rate);
    }

    emptying = 0;
    filling = 0;

    bailing = FALSE;

    valbuffer = (long *)malloc(sizeof(long) * samplesperbuf);
    if (!valbuffer) {
        fprintf(stderr, "Unable to allocate sound buffer.\n");
        return FALSE;     
    }
    memset(valbuffer, 0, sizeof(long) * samplesperbuf);

    rawbuffer = (buffer_t *)malloc(sizeof(buffer_t) * bufcount);
    memset(rawbuffer, 0, sizeof(buffer_t) * bufcount);

    for (bx=0; bx<bufcount; bx++) {
        buffer_t *buffer = &rawbuffer[bx];

        buffer->full = FALSE;

        buffer->buf = (float *)malloc(sound_buffersize);
        if (!buffer->buf) {
            fprintf(stderr, "Unable to allocate sound buffer.\n");
            /* free stuff */
            return FALSE;    
        }
        memset(buffer->buf, 0, sound_buffersize);

        res = pthread_mutex_init(&buffer->mutex, NULL);
        if (res) {
            fprintf(stderr, "Unable to init mutex.\n");
            /* free stuff */
            return FALSE;
        }

        res = pthread_cond_init(&buffer->cond, NULL);
        if (res) {
            fprintf(stderr, "Unable to init cond.\n");
            /* free stuff */
            return FALSE;
        }

    }

    status = AudioDeviceAddIOProc(audevice, PlaybackIOProc, (void *)1);

    if (status) {
        fprintf(stderr, "Could not add IOProc to device.\n");
        return FALSE;
    }

    status = AudioDeviceStart(audevice, PlaybackIOProc);
    if (status) {
        fprintf(stderr, "Could not start audio device.\n");
        return FALSE;
    }

    return TRUE;
}

void audev_close_device()
{
    OSStatus status;
    int bx;

    if (audevice == kAudioDeviceUnknown) {
        fprintf(stderr, "Unable to close sound device which was never opened.\n");
        return;
    }

    bailing = TRUE;

    for (bx=0; bx<bufcount; bx++) {
        buffer_t *buffer = &rawbuffer[bx];

        pthread_mutex_lock(&buffer->mutex);

        while (buffer->full)
            pthread_cond_wait(&buffer->cond, &buffer->mutex);

        pthread_mutex_unlock(&buffer->mutex);
    }

    status = AudioDeviceStop(audevice, PlaybackIOProc);
    if (status) {
        fprintf(stderr, "Could not stop audio device; continuing.\n");
    }

    status = AudioDeviceRemoveIOProc(audevice, PlaybackIOProc);
    if (status) {
        fprintf(stderr, "Could not remove IOProc from device.\n");
    }

    audevice = kAudioDeviceUnknown;

    for (bx=0; bx<bufcount; bx++) {
        buffer_t *buffer = &rawbuffer[bx];

        if (buffer->buf) {
            free(buffer->buf);
            buffer->buf = NULL;
        }

        pthread_mutex_destroy(&buffer->mutex);
        pthread_cond_destroy(&buffer->cond);
    }

    free(rawbuffer);

    if (valbuffer) {
        free(valbuffer);
        valbuffer = NULL;
    }

    fprintf(stderr, "Stopped audio device\n");
}

static OSStatus PlaybackIOProc(AudioDeviceID inDevice,
        const AudioTimeStamp *inNow,
        const AudioBufferList *inInputData,
        const AudioTimeStamp *inInputTime,
        AudioBufferList *outOutputData,
        const AudioTimeStamp *inOutputTime,
        void *inClientData)
{
    float *ptr = (float *) (outOutputData->mBuffers[0].mData);
    int ix;
    buffer_t *buffer;

    buffer = &rawbuffer[emptying];

    pthread_mutex_lock(&buffer->mutex);

    if (!buffer->full) {

        pthread_mutex_unlock(&buffer->mutex);

        for (ix = 0; ix < samplesperbuf; ix++)
            ptr[ix] = 0.0;

    }
    else {

        for (ix = 0; ix < samplesperbuf; ix++)
            ptr[ix] = buffer->buf[ix];

        buffer->full = FALSE;

        emptying += 1;
        if (emptying >= bufcount)
            emptying = 0;

        pthread_mutex_unlock(&buffer->mutex);
        pthread_cond_signal(&buffer->cond);

    }

    return kAudioHardwareNoError;
}

long audev_get_soundrate()
{
    return sound_rate;
}

long audev_get_framesperbuf()
{
    return framesperbuf;
}

int audev_loop(mix_func_t mixfunc, generate_func_t genfunc, void *rock)
{
    int ix, res;

    if (audevice == kAudioDeviceUnknown) {
        fprintf(stderr, "Sound device is not open.\n");
        return FALSE;
    }

    while (1) {
        float *destptr;
        long *srcptr;
        buffer_t *buffer;

        if (bailing) {
            return FALSE;
        }

        res = mixfunc(valbuffer, genfunc, rock);
        if (res) {
            bailing = TRUE;
            return TRUE;
        }

        buffer = &rawbuffer[filling];

        pthread_mutex_lock(&buffer->mutex);

        while (buffer->full)
            pthread_cond_wait(&buffer->cond, &buffer->mutex);

        srcptr = valbuffer;
        destptr = buffer->buf;

        for (ix=0; ix<samplesperbuf; ix++, srcptr++, destptr++) {
            long samp = *srcptr;
            if (samp > 0x7FFF)
                samp = 0x7FFF;
            else if (samp < -0x7FFF)
                samp = -0x7FFF;
            *destptr = ((float)samp) * (float)0.00003051757;
            /* that is, dest = (samp/32768) */
        }

        buffer->full = TRUE;

        filling += 1;
        if (filling >= bufcount) 
            filling = 0;

        pthread_mutex_unlock(&buffer->mutex);
    }
}

int audev_play_full_buff(unsigned char *buff)
{
    int ix;
    float *destptr;
    buffer_t *buffer;
    long samp;
    float val;

    if (audevice == kAudioDeviceUnknown) {
        fprintf(stderr, "Sound device is not open.\n");
        return FALSE;
    }

    buffer = &rawbuffer[filling];

    pthread_mutex_lock(&buffer->mutex);

    if (buffer->full)
    {
        pthread_mutex_unlock(&buffer->mutex);
        return FALSE;
    }

    // JW 17/08/05
    // Having this in slows the emulator right down so ignore any full buffer events rather than wait for a free buffer

    while (buffer->full)
    {
        pthread_cond_wait(&buffer->cond, &buffer->mutex);
    }

    destptr = buffer->buf;

    //  fprintf(stderr, "Removing %d samples\n", samplesperbuf / 2);

    for (ix=0; ix < (samplesperbuf / 2); ix++) 
    {
        samp = (buff[ix] - 128) * 256;
        if (samp > 0x7FFF)
            samp = 0x7FFF;
        else if (samp < -0x7FFF)
            samp = -0x7FFF;
        val = ((float)samp) * (float)0.00003051757;
        *destptr++ = val;
        *destptr++ = val;
        /* that is, dest = (samp/32768) */
    }

    buffer->full = TRUE;

    filling += 1;
    if (filling >= bufcount) 
        filling = 0;

    pthread_mutex_unlock(&buffer->mutex);

    return FALSE;

}

void audev_play_buff(unsigned char *buff, int len)
{
    static unsigned char b[32768];
    static int ptr = 0;
    int a;

    for (a = 0; a < len; ++a)
    {
        b[ptr++] = buff[a];
        if (ptr == (samplesperbuf / 2) )
        {
            audev_play_full_buff(b);
            ptr = 0;
        }
    }
}
