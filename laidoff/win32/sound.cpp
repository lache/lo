#include "platform_detection.h"
#include "lwmacro.h"
#include "lwlog.h"
#include "constants.h"
#include "sound.h"
#define LW_ENABLE_SOUND (LW_PLATFORM_WIN32 && 1)
#if LW_ENABLE_SOUND
#include <windows.h>
#include <xaudio2.h>
#include <stdlib.h>  
#include <stdio.h>  
#include <conio.h>  
//#include <process.h> 
#include <dshow.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

typedef struct _LWSOUND {
    int valid;
    char filename[MAX_PATH];
    char* buffer;
    WAVEFORMATEX wf;
    XAUDIO2_BUFFER xa;
} LWSOUND;

typedef struct _LWSOUNDSOURCE {
    WAVEFORMATEX wf;
    IXAudio2SourceVoice* source;
} LWSOUNDSOURCE;

#define MAX_MONO_SOUND_SOURCE_POOL (8)
static LWSOUNDSOURCE mono_sound_source_pool[MAX_MONO_SOUND_SOURCE_POOL];
static int mono_sound_source_pool_index;

#define MAX_STEREO_SOUND_SOURCE_POOL (8)
static LWSOUNDSOURCE stereo_sound_source_pool[MAX_STEREO_SOUND_SOURCE_POOL];
static int stereo_sound_source_pool_index;

typedef struct _LWWAVEHEADER {
    DWORD chunkID;       // 0x52494646 "RIFF" in big endian
    DWORD chunkSize;     // 4 + (8 + subChunk1Size) + (8 + subChunk2Size)
    DWORD format;        // 0x57415645 "WAVE" in big endian

    DWORD subChunk1ID;   // 0x666d7420 "fmt " in big endian
    DWORD subChunk1Size; // 16 for PCM
    WORD  audioFormat;   // 1 for PCM
    WORD  numChannels;   // 1 for mono, 2 for stereo
    DWORD sampleRate;    // 8000, 22050, 44100, etc...
    DWORD byteRate;      // sampleRate * numChannels * bitsPerSample/8
    WORD  blockAlign;    // numChannels * bitsPerSample/8
    WORD  bitsPerSample; // number of bits (8 for 8 bits, etc...)

    DWORD subChunk2ID;   // 0x64617461 "data" in big endian
    DWORD subChunk2Size; // numSamples * numChannels * bitsPerSample/8 (this is the actual data size in bytes)
} LWWAVEHEADER;

IXAudio2* g_engine = NULL;
IXAudio2MasteringVoice* g_master = NULL;

static LWSOUND sound_pool[LWS_COUNT];

int load_sound(int i) {
    int num_chan, samprate;
    short *output;
    int bits_per_samp = 16;
    unsigned int num_samples = stb_vorbis_decode_filename(SOUND_FILE[i], &num_chan, &samprate, &output);
    LOGI("Loading sfx %s...", SOUND_FILE[i]);
    LOGI(" - sfx num_samples: %u", num_samples);
    LOGI(" - sfx num_chan: %d", num_chan);
    LOGI(" - sfx samprate: %d", samprate);
    int total_payload_bytes = num_samples * num_chan * bits_per_samp / 8;
    WORD blockAlign = num_chan * bits_per_samp / 8;
    DWORD avgBytesPerSec = samprate * num_chan * bits_per_samp / 8;
    _LWWAVEHEADER wh = {
        (DWORD)0x46464952,
        (DWORD)(4 + (8 + 16) + (8 + total_payload_bytes)),
        (DWORD)0x45564157,

        (DWORD)0x20746d66,
        (DWORD)16,
        (WORD)1,
        (WORD)num_chan,
        (DWORD)samprate,
        (DWORD)avgBytesPerSec,
        blockAlign,
        (WORD)bits_per_samp,

        (DWORD)0x61746164,
        (DWORD)total_payload_bytes
    };
    char* sound_buffer = (char*)malloc(sizeof(_LWWAVEHEADER) + total_payload_bytes);
    memcpy(sound_buffer, &wh, sizeof(_LWWAVEHEADER));
    memcpy(sound_buffer + sizeof(_LWWAVEHEADER), output, total_payload_bytes);

    free(output);
    output = NULL;

    sound_pool[i].valid = 1;
    sound_pool[i].buffer = sound_buffer;
    strcpy(sound_pool[i].filename, SOUND_FILE[i]);


    sound_pool[i].wf.wFormatTag = 1; // 1 = PCM
    sound_pool[i].wf.nChannels = num_chan;
    sound_pool[i].wf.nSamplesPerSec = samprate;
    sound_pool[i].wf.nAvgBytesPerSec = avgBytesPerSec;
    sound_pool[i].wf.nBlockAlign = blockAlign;
    sound_pool[i].wf.wBitsPerSample = bits_per_samp;
    sound_pool[i].wf.cbSize = 0;

    memset(&sound_pool[i].xa, 0, sizeof(XAUDIO2_BUFFER));
    sound_pool[i].xa.pAudioData = (const BYTE*)(sound_buffer + sizeof(_LWWAVEHEADER));
    sound_pool[i].xa.AudioBytes = total_payload_bytes;

    /*
    if (FAILED(g_engine->CreateSourceVoice(&sound_pool[i].source, &sound_pool[i].wf))) {
    return -1;
    }

    sound_pool[i].source->Start();
    */

    return 0;
}

void release_sound(int i) {
    sound_pool[i].valid = 0;
    sound_pool[i].buffer = 0;
    sound_pool[i].filename[0] = '\0';
}

int create_sound_source_pool() {
    for (int i = 0; i < MAX_MONO_SOUND_SOURCE_POOL; i++) {
        mono_sound_source_pool[i].wf = sound_pool[LWS_COLLISION].wf;

        if (FAILED(g_engine->CreateSourceVoice(&mono_sound_source_pool[i].source, &mono_sound_source_pool[i].wf))) {
            return -1;
        }

        mono_sound_source_pool[i].source->Start();
    }

    for (int i = 0; i < MAX_STEREO_SOUND_SOURCE_POOL; i++) {
        stereo_sound_source_pool[i].wf = sound_pool[LWS_COLLISION].wf;

        if (FAILED(g_engine->CreateSourceVoice(&stereo_sound_source_pool[i].source, &stereo_sound_source_pool[i].wf))) {
            return -1;
        }

        stereo_sound_source_pool[i].source->Start();
    }

    return 0;
}

#else
#define HRESULT int
#endif

#if LW_PLATFORM_OSX
extern "C" void preload_all_sound_osx();
extern "C" void unload_all_sound_osx();
#endif

extern "C" HRESULT init_ext_sound_lib() {
#if LW_PLATFORM_WIN32 && LW_ENABLE_SOUND

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (FAILED(XAudio2Create(&g_engine))) {
        CoUninitialize();
        return -1;
    }

    if (FAILED(g_engine->CreateMasteringVoice(&g_master))) {
        g_engine->Release();
        CoUninitialize();
        return -2;
    }

    for (int i = 0; i < LWS_COUNT; i++) {
        load_sound(i);
    }

    create_sound_source_pool();

    //BOOL r = PlaySound(sound_buffer, NULL, SND_MEMORY | SND_ASYNC);

    return S_OK;
#elif LW_PLATFORM_OSX
    preload_all_sound_osx();
    return 0;
#else
    return 0;
#endif
}

extern "C" void destroy_ext_sound_lib() {
#if LW_ENABLE_SOUND
    for (int i = 0; i < LWS_COUNT; i++) {
        release_sound(i);
    }
#elif LW_PLATFORM_OSX
    unload_all_sound_osx();
#else
#endif
}

#if LW_PLATFORM_OSX
extern "C" void play_sound_osx(LW_SOUND lws);
#endif

extern "C" void play_sound(LW_SOUND lws) {
#if LW_ENABLE_SOUND
    if (sound_pool[lws].wf.nChannels == 1) {
        mono_sound_source_pool[mono_sound_source_pool_index].source->SubmitSourceBuffer(&sound_pool[lws].xa);
        mono_sound_source_pool_index = (mono_sound_source_pool_index + 1) % MAX_MONO_SOUND_SOURCE_POOL;
    } else if (sound_pool[lws].wf.nChannels == 2) {
        stereo_sound_source_pool[stereo_sound_source_pool_index].source->SubmitSourceBuffer(&sound_pool[lws].xa);
        stereo_sound_source_pool_index = (stereo_sound_source_pool_index + 1) % MAX_STEREO_SOUND_SOURCE_POOL;
    }
    // VERY SLOW win32 VERSION - DO NOT USE
    //PlaySound(sound_pool[lws].buffer, NULL, SND_MEMORY | SND_ASYNC);
#elif LW_PLATFORM_OSX
    play_sound_osx(lws);
#endif
}

extern "C" void stop_sound(LW_SOUND lws) {
}
