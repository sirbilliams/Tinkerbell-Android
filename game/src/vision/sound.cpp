#include "sound.h"
#include "tb.h"
#include <SDL.h>
#include <string.h>
#include <algorithm>

namespace Vision {

static std::string g_audio_dir = "audio";   // port: the original's globals (0x5a74d8+4, 0x5a758c) hold this
void sound_set_audio_dir(const char *dir) { g_audio_dir = dir ? dir : ""; }
const std::string &sound_audio_dir() { return g_audio_dir; }

static std::vector<Sound *> g_active;       // sounds whose stream wants update() calls

// ---------------------------------------------------------------- SoundBuffer

bool SoundBuffer::load() {                  // 0x41f560
    free();
    std::string rel = g_audio_dir.empty() ? name + ".wv6" : g_audio_dir + "\\" + name + ".wv6";
    FILE *f = tb_fopen_ci(rel.c_str());     // port: Vision::File::open (case-insensitive game folder)
    if (!f) {
        tb_log("SoundBuffer: could not load %s", name.c_str());
        return false;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (uint8_t *)malloc((size_t)n + 16);   // port: slack so the decoder can read one byte ahead
    if (!data) { fclose(f); return false; }
    size = (uint32_t)fread(data, 1, (size_t)n, f);
    memset(data + size, 0, 16);
    fclose(f);
    return true;
}

void SoundBuffer::free() {                  // 0x420490
    ::free(data);
    data = nullptr;
    size = 0;
}

// ---------------------------------------------------------------- SoundStream

SoundStream::SoundStream()
    : ds(nullptr), size(0), loop(false), fromFile(false), file(nullptr), wv(nullptr), writeOfs(0), half(0),
      ended(false), finished(false) {}      // 0x420d40

void SoundStream::init(TbDSBuffer *b, uint32_t sz, Wv6 *w, bool lp, const char *fname) {   // 0x420b90
    if (fname) {
        fromFile = true;
        std::string dir = sound_audio_dir();
        fileName = dir.empty() ? std::string(fname) + ".wv6" : dir + "\\" + fname + ".wv6";
    }
    wv = w;
    loop = lp;
    w->loop = lp;                           // 0x25c
    ds = b;
    size = sz;
    if (fromFile) {
        if (file) fclose(file);
        file = tb_fopen_ci(fileName.c_str());
    }
}

bool SoundStream::refill() {                // 0x4209f0
    uint32_t play = 0;
    ds->GetCurrentPosition(&play, nullptr);
    uint32_t h = size / 2;
    bool fill = false;
    if (half == 0 && play > h) { writeOfs = 0; fill = true; }
    if (half == 1 && play < h) writeOfs = h;
    else if (!fill) return true;
    void *p1 = nullptr, *p2 = nullptr; uint32_t n1 = 0, n2 = 0;
    ds->Lock(writeOfs, h, &p1, &n1, &p2, &n2, 0);
    uint32_t samples = size / 4;            // (size + (size>>31 & 3)) >> 2
    bool r = fromFile ? wv->readFile((int16_t *)p1, samples, file) : wv->readMem((int16_t *)p1, samples);
    ds->Unlock(p1, h, p2, n2);
    half = half == 0 ? 1 : 0;
    return loop ? true : r;
}

void SoundStream::prime() {                 // 0x420ae0
    // port: a fresh stream starts from the first sample (the original reset the reader in makeBuffer / on play)
    void *p1 = nullptr, *p2 = nullptr; uint32_t n1 = 0, n2 = 0;
    ds->Lock(0, size, &p1, &n1, &p2, &n2, 0);
    wv->reset();
    if (fromFile) wv->readFile((int16_t *)p1, size / 2, file);
    else wv->readMem((int16_t *)p1, size / 2);
    ds->Unlock(p1, size, p2, n2);
    half = 0;
}

void SoundStream::start() {                 // 0x420c60
    prime();
    ended = false;
    finished = false;
    ds->Play(0, 0, TB_DSBPLAY_LOOPING);
}

void SoundStream::update() {                // 0x420c80
    if (finished) return;
    if (!ended) {
        if (refill()) return;
        ds->Play(0, 0, 0);                  // the data ran out: let the buffer play to its end once
        ended = true;
    }
    uint32_t st = 0;
    ds->GetStatus(&st);
    if (st & TB_DSBSTATUS_PLAYING) finished = true;   // as the original tests it (bit 0 set)
}

void SoundStream::stop() {                  // 0x420cf0
    if (ds) ds->Stop();
    finished = true;
}

void SoundStream::release() {               // 0x420d10
    if (ds) ds->Stop();
    finished = true;
    if (fromFile && file) { fclose(file); file = nullptr; }
}

// ---------------------------------------------------------------- Sound

Sound::Sound(const char *n)                 // 0x420210
    : name(n ? n : ""), mode(0), field33c(5), type(7), playedAt(0), buf_(nullptr), shared_(nullptr),
      ds_(nullptr), valid_(false), opened_(false) {
    own_.name = name;
}

Sound::~Sound() {                           // 0x420310
    close();
}

bool Sound::makeDirectSoundBuffer(uint32_t bytes) {   // 0x41ff00
    TbWaveFormat wf;
    memset(&wf, 0, sizeof wf);
    uint16_t blockAlign = (uint16_t)((wv_.channels * wv_.bits) >> 3);
    wf.tag = 1; wf.channels = (uint16_t)wv_.channels; wf.rate = wv_.rate;
    wf.avgBytes = blockAlign * wv_.rate; wf.blockAlign = blockAlign; wf.bits = (uint16_t)wv_.bits;
    TbDSBufferDesc desc;
    memset(&desc, 0, sizeof desc);
    desc.size = 0x24; desc.flags = 0x1e2; desc.bytes = bytes; desc.fmt = &wf;
    if (!g_tb_dsound || g_tb_dsound->CreateSoundBuffer(&desc, &ds_) != 0) {
        ds_ = nullptr;
        tb_log("CreateSoundBuffer failed in makeDirectSoundBuffer");
        return false;
    }
    void *p1 = nullptr, *p2 = nullptr; uint32_t n1 = 0, n2 = 0;
    ds_->Lock(0, bytes, &p1, &n1, &p2, &n2, 0);
    memset(p1, 0, bytes);
    ds_->Unlock(p1, bytes, nullptr, 0);
    return true;
}

void Sound::makeBuffer() {                  // 0x420040 (mode 1 and 2)
    if (mode == 1) {
        wv_.parse(buf_->data);              // 0x422f30
        wv_.reset();
        uint32_t bytes = wv_.rate == 0 ? 0x16000 : (wv_.bits >> 3) * wv_.rate;
        makeDirectSoundBuffer(bytes);
        return;
    }
    if (mode == 2) {                        // Sound::makeStreamBuffer
        std::string dir = sound_audio_dir();
        std::string rel = dir.empty() ? name + ".wv6" : dir + "\\" + name + ".wv6";
        FILE *f = tb_fopen_ci(rel.c_str());
        if (!f) {
            tb_log("Sound::makeStreamBuffer could not load %s", name.c_str());
            valid_ = false;
            return;
        }
        wv_.readHeader(f);                  // 0x423060
        wv_.reset();
        fclose(f);
        uint32_t bytes = wv_.rate == 0 ? 0x16000 : (wv_.bits >> 3) * wv_.rate;
        makeDirectSoundBuffer(bytes);
        valid_ = true;
    }
}

void Sound::open(SoundBuffer *shared) {     // 0x41f9d0
    if (name.empty()) {
        tb_log("WARNING: Sound::open() tried to open an no-name sound");
        return;
    }
    playedAt = 0;
    extra.clear();
    valid_ = false;
    if (!g_tb_dsound) return;
    bool owns = (shared == nullptr);
    if (mode != 2) {
        if (owns) {
            own_.name = name;
            own_.load();
            if (!own_.data) return;
            valid_ = true;
            own_.users++;
            buf_ = &own_;
        } else {
            if (!shared->data) return;
            valid_ = true;
            shared_ = shared;
            shared->users++;
            buf_ = shared_;
        }
    }
    if (mode == 0) {
        wv_.parse(buf_->data);
        if (makeDirectSoundBuffer(wv_.bytes)) {
            wv_.reset();
            void *p1 = nullptr, *p2 = nullptr; uint32_t n1 = 0, n2 = 0;
            ds_->Lock(0, wv_.bytes, &p1, &n1, &p2, &n2, 0);
            wv_.readAll(p1);
            ds_->Unlock(p1, wv_.bytes, p2, n2);
        }
    } else if (mode == 1 || mode == 2) {
        makeBuffer();
        if (valid_ && ds_) {
            uint32_t bytes = wv_.rate == 0 ? 0x16000 : (wv_.bits >> 3) * wv_.rate;
            stream_.init(ds_, bytes, &wv_, type == 6, mode == 2 ? name.c_str() : nullptr);
        }
    }
    if (owns && mode == 0) {
        if (wv_.len2 && wv_.data2) extra.assign(wv_.data2, wv_.data2 + wv_.len2);   // keep block 2 (+0x330)
        own_.free();                        // the decoded sound lives in the DirectSound buffer now
        own_.users--;
        buf_ = nullptr;
    }
    opened_ = true;
}

void Sound::play() {                        // 0x41fd50
    if (!g_tb_dsound) return;
    if (!valid_) {
        tb_log("Cannot play sound %s (it's not valid).", wv_.name);
        return;
    }
    playedAt = SDL_GetTicks();
    if (mode == 0) {
        if (g_tb_dsound && valid_ && ds_) ds_->SetCurrentPosition(0);
        if (ds_) ds_->Play(0, 0, type == 6 ? TB_DSBPLAY_LOOPING : 0);
    } else if (mode == 1 || mode == 2) {
        if (ds_) {
            stream_.start();                // 0x420c60
            if (std::find(g_active.begin(), g_active.end(), this) == g_active.end()) g_active.push_back(this);   // service start
        }
    }
}

void Sound::stop() {                        // 0x41fe70
    if (g_tb_dsound && valid_) {
        if (mode == 1 || mode == 2) {
            stream_.stop();                 // 0x420cf0
            g_active.erase(std::remove(g_active.begin(), g_active.end(), this), g_active.end());   // service stop
        } else if (ds_) {
            ds_->Stop();
        }
    }
}

void Sound::close() {                       // 0x41fc50
    if (opened_) {
        if (g_tb_dsound && valid_) {
            if (mode == 1 || mode == 2) {
                stream_.stop();
                g_active.erase(std::remove(g_active.begin(), g_active.end(), this), g_active.end());
            } else if (ds_) {
                ds_->Stop();
            }
            if (mode == 1 || mode == 2) stream_.release();   // 0x420d10
            if (mode != 2) {
                if (buf_) buf_->users--;
                if (shared_ == nullptr) own_.free();         // our own copy of the record
                shared_ = nullptr;
                buf_ = nullptr;
            }
            if (ds_) { ds_->Release(); ds_ = nullptr; }
            extra.clear();
            opened_ = false;
            valid_ = false;
            return;
        }
        opened_ = false;
    }
}

bool Sound::isPlaying() {                   // 0x41fec0
    if (!g_tb_dsound || !valid_ || !ds_) return false;
    uint32_t st = 0;
    ds_->GetStatus(&st);
    return (st & TB_DSBSTATUS_PLAYING) != 0;
}

uint32_t Sound::lengthMs() {                // 0x41fe30
    if (wv_.rate == 0) return 0;
    return wv_.frames() * 1000u / wv_.rate;
}

void Sound::tickAll() {
    std::vector<Sound *> snap = g_active;
    for (size_t i = 0; i < snap.size(); i++) {
        Sound *s = snap[i];
        if (std::find(g_active.begin(), g_active.end(), s) == g_active.end()) continue;
        s->stream_.update();
        if (s->stream_.finished)            // nothing more to feed; the buffer plays out by itself
            g_active.erase(std::remove(g_active.begin(), g_active.end(), s), g_active.end());
    }
}

int Sound::activeCount() { return (int)g_active.size(); }

}  // namespace Vision
