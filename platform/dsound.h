// DirectSound-like shim on SDL audio (lane B). Imitates only what the game uses of IDirectSound /
// IDirectSoundBuffer: CreateSoundBuffer, and on a buffer Lock/Unlock/Play/Stop/GetStatus/
// GetCurrentPosition/SetCurrentPosition/SetVolume/SetPan/SetFrequency/Release (the vtable offsets the
// decompile calls: +0x2c Lock, +0x30 Play, +0x34 SetCurrentPosition, +0x3c SetVolume, +0x40 SetPan,
// +0x44 SetFrequency, +0x48 Stop, +0x4c Unlock, +0x10 GetCurrentPosition, +0x24 GetStatus, +0x08 Release).
// All methods return 0 on success (DS_OK), like the HRESULTs the game tests with "!= 0".
#pragma once
#include <stdint.h>
#include <stddef.h>

#define TB_DSBPLAY_LOOPING     1u
#define TB_DSBSTATUS_PLAYING   1u
#define TB_DSBSTATUS_LOOPING   4u
#define TB_DSBLOCK_FROMWRITECURSOR 1u
#define TB_DSBLOCK_ENTIREBUFFER    2u

struct TbWaveFormat {                 // WAVEFORMATEX
    uint16_t tag;                     // 1 = PCM
    uint16_t channels;
    uint32_t rate;
    uint32_t avgBytes;
    uint16_t blockAlign;
    uint16_t bits;
    uint16_t cbSize;
};

struct TbDSBufferDesc {               // DSBUFFERDESC
    uint32_t size;                    // 0x24
    uint32_t flags;                   // the game passes 0x1e2
    uint32_t bytes;
    uint32_t reserved;
    const TbWaveFormat *fmt;
};

class TbDSBuffer {
public:
    int GetCurrentPosition(uint32_t *play, uint32_t *write);
    int GetStatus(uint32_t *status);
    int Lock(uint32_t offset, uint32_t bytes, void **p1, uint32_t *n1, void **p2, uint32_t *n2, uint32_t flags);
    int Unlock(void *p1, uint32_t n1, void *p2, uint32_t n2);
    int Play(uint32_t reserved, uint32_t priority, uint32_t flags);
    int Stop();
    int SetCurrentPosition(uint32_t pos);
    int SetVolume(int32_t hundredthsDb);     // -10000..0
    int SetPan(int32_t pan);                 // -10000..10000
    int SetFrequency(uint32_t hz);           // 0 = the buffer's own rate
    void Release();                          // frees the buffer
    struct Impl;
private:
    TbDSBuffer() {}
    ~TbDSBuffer() {}
    friend class TbDSound;
    Impl *d = nullptr;
};

class TbDSound {
public:
    int CreateSoundBuffer(const TbDSBufferDesc *desc, TbDSBuffer **out);   // 0 = ok
};

// The game keeps the IDirectSound pointer in a global (0x5a751c): null = no sound. Created once at start
// (the original calls DirectSoundCreate inside the redirected API of WinMain/Stage init).
extern TbDSound *g_tb_dsound;
bool tb_dsound_create();                 // opens the SDL audio device; false (and g_tb_dsound stays null) if none
void tb_dsound_destroy();
int  tb_dsound_device_rate();            // output rate (stereo s16)

// Render the mix into out (stereo s16, interleaved). The SDL callback calls this; tests may call it directly
// when no device is open (tb_dsound_create_offline()).
void tb_dsound_mix(int16_t *out, int frames);
bool tb_dsound_create_offline(int rate);  // no SDL device; the caller pulls with tb_dsound_mix

// A queue of stereo s16 frames at the device rate, mixed with the buffers (the movie's soundtrack).
struct TbPcmStream;
TbPcmStream *tb_pcmstream_create();
void tb_pcmstream_destroy(TbPcmStream *s);
void tb_pcmstream_push(TbPcmStream *s, const int16_t *stereo, int frames);
int  tb_pcmstream_queued(TbPcmStream *s);             // frames waiting
int64_t tb_pcmstream_played(TbPcmStream *s);          // frames consumed by the mixer so far
void tb_pcmstream_clear(TbPcmStream *s);

// Case-insensitive open of a game-folder path ('\' or '/'), like the original's Windows file system.
// port: stands in for Vision::File's open until lane A's File is the one used everywhere.
#include <stdio.h>
FILE *tb_fopen_ci(const char *rel);
