// Vision::Sound / SoundBuffer / SoundStream (lane B), reconstructed from
//   SoundBuffer: 0x41f560 (load a .wv6 record), 0x420490 (free)
//   Sound:       0x420210 ctor, 0x420310 dtor, 0x41f9d0 open, 0x41fd50 play, 0x41fe70 stop,
//                0x41fc50 close, 0x41fec0 isPlaying, 0x41fe30 length, 0x41ff00 makeDirectSoundBuffer,
//                0x420040 makeBuffer/makeStreamBuffer
//   SoundStream: 0x420b90 init, 0x4209f0 refill a half, 0x420ae0 prime, 0x420c60 start, 0x420c80 update,
//                0x420cf0 stop, 0x420d10 release (the object embedded at Sound+0x6c)
// Interface for the game classes (the original calls these as plain functions on a Sound*):
//   Sound *s = new Sound("nvmp_mu_music"); s->mode = 2 (stream from file), s->type = 6 (loop);
//   s->open(); s->play(); ... s->stop(); s->close(); delete s;
//   Sound::tickAll() once per frame/tick feeds the streaming sounds (the original's Service tick).
#pragma once
#include "wv6.h"
#include "../../../platform/dsound.h"
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace Vision {

// Where the .wv6 files are (the original keeps these in globals set by WinMain; default "audio").
void sound_set_audio_dir(const char *dir);
const std::string &sound_audio_dir();

class SoundBuffer {                          // 0x41f560 / 0x420490 (vtable 0x513ac0)
public:
    SoundBuffer() : data(nullptr), size(0), users(0) {}
    ~SoundBuffer() { free(); }
    std::string name;                        // set before load()
    uint8_t *data;                           // +0x24: the raw .wv6 record
    uint32_t size;                           // +0x2c
    int users;                               // +0x28: Sounds sharing this buffer
    bool load();                             // 0x41f560: "<audio>\<name>.wv6"
    void free();                             // 0x420490
};

class SoundStream {                          // the streaming helper embedded in Sound (+0x6c)
public:
    SoundStream();
    TbDSBuffer *ds;                          // +4
    uint32_t size;                           // +8: DirectSound buffer bytes
    bool loop;                               // +0xc
    bool fromFile;                           // +0xd
    std::string fileName;                    // +0x10: the File it reads
    FILE *file;                              // +0x10 + 0x24
    Wv6 *wv;                                 // +0x54
    uint32_t writeOfs;                       // +0
    int half;                                // +0x50: which half is refilled next
    bool ended, finished;                    // +0x58, +0x59
    void init(TbDSBuffer *ds, uint32_t size, Wv6 *wv, bool loop, const char *file);   // 0x420b90
    bool refill();                           // 0x4209f0
    void prime();                            // 0x420ae0
    void start();                            // 0x420c60
    void update();                           // 0x420c80
    void stop();                             // 0x420cf0
    void release();                          // 0x420d10
};

class Sound {                                // vtable 0x513a94, 0x348 bytes
public:
    explicit Sound(const char *name);        // 0x420210
    ~Sound();                                // 0x420310

    std::string name;
    int mode;                                // +0x338: 0 = decode whole into the buffer, 1 = stream from memory, 2 = stream from the file
    int field33c;                            // +0x33c (5)
    int type;                                // +0x340 (7); 6 = loops
    uint32_t playedAt;                       // +0x344: tick of the last play()

    void open(SoundBuffer *shared = nullptr);   // 0x41f9d0
    void play();                             // 0x41fd50
    void stop();                             // 0x41fe70
    void close();                            // 0x41fc50
    bool isPlaying();                        // 0x41fec0
    uint32_t lengthMs();                     // 0x41fe30
    bool valid() const { return valid_; }

    // the optional second data block of the record, kept after the ADPCM data is freed (+0x330 / +0x31c)
    std::vector<uint8_t> extra;

    static void tickAll();                   // port: the Service tick that calls SoundStream::update()
    static int activeCount();

private:
    bool makeDirectSoundBuffer(uint32_t bytes);   // 0x41ff00
    void makeBuffer();                       // 0x420040
    SoundBuffer own_;                        // +0x38
    SoundBuffer *buf_;                       // +0x68: the buffer being used
    SoundBuffer *shared_;                    // +0x34
    Wv6 wv_;                                 // +0xd0
    SoundStream stream_;                     // +0x6c
    TbDSBuffer *ds_;                         // +0xc8 -> +4
    bool valid_;                             // +0xcc
    bool opened_;                            // +0x334
    friend struct SoundTicker;
};

}  // namespace Vision
