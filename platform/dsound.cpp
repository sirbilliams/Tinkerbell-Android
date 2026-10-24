// DirectSound-like shim on SDL audio (lane B). See dsound.h.
#include "dsound.h"
#include "tb.h"
#include <SDL.h>
#include <dirent.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <mutex>
#include <string>
#include <vector>

TbDSound *g_tb_dsound = nullptr;

namespace {
std::mutex g_mx;
std::vector<TbDSBuffer::Impl *> g_bufs;
std::vector<TbPcmStream *> g_streams;
SDL_AudioDeviceID g_dev = 0;
int g_rate = 44100;
TbDSound g_ds_obj;
}

struct TbDSBuffer::Impl {
    TbDSBuffer iface;
    std::vector<uint8_t> data;
    TbWaveFormat fmt;
    uint32_t blockAlign = 2;
    double pos = 0;             // in frames
    bool playing = false, looping = false;
    int32_t volume = 0, pan = 0;
    uint32_t freq = 0;
};

struct TbPcmStream {
    std::vector<int16_t> q;     // stereo frames
    size_t head = 0;            // frames already consumed (compacted lazily)
    int64_t played = 0;
};

static float db_gain(int32_t hundredths) {
    if (hundredths <= -10000) return 0.f;
    if (hundredths >= 0) return 1.f;
    return powf(10.f, (float)hundredths / 2000.f);
}

static inline int16_t clip16(int32_t v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v; }

// ---- IDirectSoundBuffer

int TbDSBuffer::GetCurrentPosition(uint32_t *play, uint32_t *write) {
    std::lock_guard<std::mutex> l(g_mx);
    uint32_t p = (uint32_t)d->pos * d->blockAlign;
    if (play) *play = p;
    if (write) {   // port: DirectSound's write cursor leads the play cursor by its mixing latency
        uint32_t lead = d->fmt.avgBytes / 20;
        lead -= lead % d->blockAlign;
        *write = d->data.empty() ? 0 : (p + lead) % (uint32_t)d->data.size();
    }
    return 0;
}

int TbDSBuffer::GetStatus(uint32_t *status) {
    std::lock_guard<std::mutex> l(g_mx);
    *status = d->playing ? (TB_DSBSTATUS_PLAYING | (d->looping ? TB_DSBSTATUS_LOOPING : 0)) : 0;
    return 0;
}

int TbDSBuffer::Lock(uint32_t offset, uint32_t bytes, void **p1, uint32_t *n1, void **p2, uint32_t *n2, uint32_t flags) {
    uint32_t size = (uint32_t)d->data.size();
    if (flags & TB_DSBLOCK_ENTIREBUFFER) { offset = 0; bytes = size; }
    if (offset >= size || bytes > size) return -1;   // DSERR_INVALIDPARAM
    uint32_t a = bytes;
    if (offset + a > size) a = size - offset;
    *p1 = &d->data[offset]; *n1 = a;
    if (a < bytes) { if (p2) *p2 = &d->data[0]; if (n2) *n2 = bytes - a; }
    else { if (p2) *p2 = nullptr; if (n2) *n2 = 0; }
    return 0;
}

int TbDSBuffer::Unlock(void *, uint32_t, void *, uint32_t) { return 0; }

int TbDSBuffer::Play(uint32_t, uint32_t, uint32_t flags) {
    std::lock_guard<std::mutex> l(g_mx);
    d->playing = true;
    d->looping = (flags & TB_DSBPLAY_LOOPING) != 0;
    return 0;
}

int TbDSBuffer::Stop() {
    std::lock_guard<std::mutex> l(g_mx);
    d->playing = false;      // the play cursor stays where it is
    return 0;
}

int TbDSBuffer::SetCurrentPosition(uint32_t pos) {
    std::lock_guard<std::mutex> l(g_mx);
    if (pos >= d->data.size()) pos = 0;
    d->pos = pos / d->blockAlign;
    return 0;
}

int TbDSBuffer::SetVolume(int32_t v) { std::lock_guard<std::mutex> l(g_mx); d->volume = v; return 0; }
int TbDSBuffer::SetPan(int32_t p) { std::lock_guard<std::mutex> l(g_mx); d->pan = p; return 0; }
int TbDSBuffer::SetFrequency(uint32_t hz) { std::lock_guard<std::mutex> l(g_mx); d->freq = hz; return 0; }

void TbDSBuffer::Release() {
    Impl *i = d;
    {
        std::lock_guard<std::mutex> l(g_mx);
        for (size_t k = 0; k < g_bufs.size(); k++)
            if (g_bufs[k] == i) { g_bufs.erase(g_bufs.begin() + k); break; }
    }
    delete i;
}

// ---- IDirectSound

int TbDSound::CreateSoundBuffer(const TbDSBufferDesc *desc, TbDSBuffer **out) {
    *out = nullptr;
    if (!desc || !desc->fmt || desc->bytes == 0 || desc->fmt->tag != 1) return -1;
    if (desc->fmt->channels < 1 || desc->fmt->channels > 2 || desc->fmt->bits != 16 || desc->fmt->rate == 0) return -1;
    TbDSBuffer::Impl *i = new TbDSBuffer::Impl();
    i->fmt = *desc->fmt;
    i->blockAlign = desc->fmt->channels * 2u;
    i->data.assign(desc->bytes - desc->bytes % i->blockAlign, 0);
    i->iface.d = i;
    std::lock_guard<std::mutex> l(g_mx);
    g_bufs.push_back(i);
    *out = &i->iface;
    return 0;
}

// ---- mixer

void tb_dsound_mix(int16_t *out, int frames) {
    std::lock_guard<std::mutex> l(g_mx);
    std::vector<int32_t> acc((size_t)frames * 2, 0);
    for (size_t b = 0; b < g_bufs.size(); b++) {
        TbDSBuffer::Impl *s = g_bufs[b];
        if (!s->playing || s->data.empty()) continue;
        const int16_t *src = (const int16_t *)&s->data[0];
        const int nch = s->fmt.channels;
        const double total = (double)(s->data.size() / s->blockAlign);
        const double step = (double)(s->freq ? s->freq : s->fmt.rate) / (double)g_rate;
        float gl = db_gain(s->volume), gr = gl;
        if (s->pan > 0) gr *= db_gain(-s->pan); else if (s->pan < 0) gl *= db_gain(s->pan);
        double pos = s->pos;
        for (int f = 0; f < frames; f++) {
            if (pos >= total) {
                if (s->looping) pos -= total;
                else { s->playing = false; pos = 0; break; }
            }
            size_t i0 = (size_t)pos;
            size_t i1 = i0 + 1; if (i1 >= (size_t)total) i1 = s->looping ? 0 : i0;
            float fr = (float)(pos - (double)i0);
            float l0, r0, l1, r1;
            if (nch == 1) { l0 = r0 = src[i0]; l1 = r1 = src[i1]; }
            else { l0 = src[i0 * 2]; r0 = src[i0 * 2 + 1]; l1 = src[i1 * 2]; r1 = src[i1 * 2 + 1]; }
            acc[f * 2]     += (int32_t)((l0 + (l1 - l0) * fr) * gl);
            acc[f * 2 + 1] += (int32_t)((r0 + (r1 - r0) * fr) * gr);
            pos += step;
        }
        s->pos = pos;
    }
    for (size_t k = 0; k < g_streams.size(); k++) {
        TbPcmStream *s = g_streams[k];
        size_t avail = s->q.size() / 2 - s->head;
        size_t n = avail < (size_t)frames ? avail : (size_t)frames;
        for (size_t f = 0; f < n; f++) {
            acc[f * 2]     += s->q[(s->head + f) * 2];
            acc[f * 2 + 1] += s->q[(s->head + f) * 2 + 1];
        }
        s->head += n; s->played += (int64_t)n;
        if (s->head > 65536) {   // compact
            s->q.erase(s->q.begin(), s->q.begin() + s->head * 2);
            s->head = 0;
        }
    }
    for (int f = 0; f < frames * 2; f++) out[f] = clip16(acc[f]);
}

static void SDLCALL audio_cb(void *, Uint8 *stream, int len) {
    tb_dsound_mix((int16_t *)stream, len / 4);
}

bool tb_dsound_create() {
    if (g_tb_dsound) return true;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { tb_log("dsound: SDL audio init failed: %s", SDL_GetError()); return false; }
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024; want.callback = audio_cb;
    g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!g_dev) { tb_log("dsound: no audio device: %s", SDL_GetError()); return false; }
    if (have.format != AUDIO_S16SYS || have.channels != 2) {   // asked for no changes; refuse anything else
        tb_log("dsound: unsupported device format"); SDL_CloseAudioDevice(g_dev); g_dev = 0; return false;
    }
    g_rate = have.freq;
    SDL_PauseAudioDevice(g_dev, 0);
    g_tb_dsound = &g_ds_obj;
    tb_log("dsound: %d Hz stereo, driver %s", g_rate, SDL_GetCurrentAudioDriver());
    return true;
}

bool tb_dsound_create_offline(int rate) {
    g_rate = rate;
    g_tb_dsound = &g_ds_obj;
    return true;
}

void tb_dsound_destroy() {
    if (g_dev) { SDL_CloseAudioDevice(g_dev); g_dev = 0; }
    g_tb_dsound = nullptr;
}

int tb_dsound_device_rate() { return g_rate; }

// ---- PCM streams

TbPcmStream *tb_pcmstream_create() {
    TbPcmStream *s = new TbPcmStream();
    std::lock_guard<std::mutex> l(g_mx);
    g_streams.push_back(s);
    return s;
}
void tb_pcmstream_destroy(TbPcmStream *s) {
    if (!s) return;
    { std::lock_guard<std::mutex> l(g_mx);
      for (size_t k = 0; k < g_streams.size(); k++) if (g_streams[k] == s) { g_streams.erase(g_streams.begin() + k); break; } }
    delete s;
}
void tb_pcmstream_push(TbPcmStream *s, const int16_t *st, int frames) {
    std::lock_guard<std::mutex> l(g_mx);
    s->q.insert(s->q.end(), st, st + (size_t)frames * 2);
}
int tb_pcmstream_queued(TbPcmStream *s) { std::lock_guard<std::mutex> l(g_mx); return (int)(s->q.size() / 2 - s->head); }
int64_t tb_pcmstream_played(TbPcmStream *s) { std::lock_guard<std::mutex> l(g_mx); return s->played; }
void tb_pcmstream_clear(TbPcmStream *s) { std::lock_guard<std::mutex> l(g_mx); s->q.clear(); s->head = 0; }

// ---- case-insensitive open (port: Windows paths are case-insensitive, Linux/Android are not)

static bool ci_find(const std::string &dir, const std::string &name, std::string &found) {
    DIR *d = opendir(dir.c_str());
    if (!d) return false;
    bool ok = false;
    while (struct dirent *e = readdir(d)) {
        if (strcasecmp(e->d_name, name.c_str()) == 0) { found = e->d_name; ok = true; break; }
    }
    closedir(d);
    return ok;
}

FILE *tb_fopen_ci(const char *rel) {
    std::string full = tb_game_path(rel);
    if (FILE *f = fopen(full.c_str(), "rb")) return f;
    // walk the path below the game folder, fixing the case of each component
    std::string base = tb_game_dir().empty() ? std::string(".") : tb_game_dir();
    std::string r;
    for (const char *c = rel; *c; c++) r += *c == '\\' ? '/' : *c;
    std::string cur = base;
    size_t i = 0;
    while (i < r.size()) {
        size_t j = r.find('/', i);
        std::string comp = r.substr(i, j == std::string::npos ? std::string::npos : j - i);
        i = j == std::string::npos ? r.size() : j + 1;
        if (comp.empty()) continue;
        std::string found;
        if (!ci_find(cur, comp, found)) return nullptr;
        cur += "/" + found;
    }
    return fopen(cur.c_str(), "rb");
}
