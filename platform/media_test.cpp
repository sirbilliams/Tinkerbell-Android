// Desktop test entry for sound and movies (lane B). Run by main() through the weak tb_test_hook
// before tb_game_main, so it does not clash with lane A's tb_game_main:
//   TB_TEST_SOUND=<name> [TB_TEST_MODE=0|1|2] [TB_TEST_SECONDS=n] [TB_TEST_OUT=dir]
//       decodes audio/<name>.wv6 with the game's decoder, writes <out>/<name>.decode.wav and
//       <out>/<name>.mode<m>.wav (the offline mixer's output through Sound), and prints sample counts.
//   TB_TEST_MOVIE=<name> [TB_TEST_OUT=dir]
//       plays data/<name>.mov through QTMovie (real time) and saves a few frames as <out>/<name>_NN.bmp.
#include "tb.h"
#include "dsound.h"
#include "../game/src/vision/sound.h"
#include "../game/src/vision/qtmovie.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static void write_wav(const std::string &path, const std::vector<int16_t> &pcm, int rate, int ch) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) { tb_log("test: cannot write %s", path.c_str()); return; }
    uint32_t dlen = (uint32_t)(pcm.size() * 2), v;
    fwrite("RIFF", 1, 4, f); v = 36 + dlen; fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); v = 16; fwrite(&v, 4, 1, f);
    uint16_t s = 1; fwrite(&s, 2, 1, f); s = (uint16_t)ch; fwrite(&s, 2, 1, f);
    v = (uint32_t)rate; fwrite(&v, 4, 1, f); v = (uint32_t)(rate * ch * 2); fwrite(&v, 4, 1, f);
    s = (uint16_t)(ch * 2); fwrite(&s, 2, 1, f); s = 16; fwrite(&s, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&dlen, 4, 1, f);
    fwrite(pcm.data(), 2, pcm.size(), f);
    fclose(f);
}

static int test_sound(const char *name) {
    const char *e;
    std::string out = (e = getenv("TB_TEST_OUT")) ? e : ".";
    int mode = (e = getenv("TB_TEST_MODE")) ? atoi(e) : 0;
    double secs = (e = getenv("TB_TEST_SECONDS")) ? atof(e) : 5.0;

    // 1) the decoder alone
    Vision::SoundBuffer sb;
    sb.name = name;
    if (!sb.load()) return 2;
    Vision::Wv6 w;
    w.parse(sb.data);
    std::vector<int16_t> pcm(w.bytes / 2 + 8);
    w.readAll(pcm.data());
    pcm.resize(w.bytes / 2);
    tb_log("test: %s: %u ch, %u bits, %u Hz, record %u bytes, adpcm %u bytes -> decoded %zu samples (header says %u bytes = %u frames)",
           name, w.channels, w.bits, w.rate, sb.size, w.len1, pcm.size(), w.bytes, w.frames());
    int16_t mn = 0, mx = 0;
    for (size_t i = 0; i < pcm.size(); i++) { if (pcm[i] < mn) mn = pcm[i]; if (pcm[i] > mx) mx = pcm[i]; }
    tb_log("test: sample range %d..%d", mn, mx);
    write_wav(out + "/" + name + ".decode.wav", pcm, (int)w.rate, (int)w.channels);

    // 2) through Sound and the offline mixer
    tb_dsound_create_offline(44100);
    Vision::Sound s(name);
    s.mode = mode;
    s.type = 6;                                  // loop
    s.open();
    tb_log("test: Sound mode %d valid=%d length %u ms", mode, (int)s.valid(), s.lengthMs());
    s.play();
    int frames = (int)(secs * 44100);
    std::vector<int16_t> mix((size_t)frames * 2);
    for (int done = 0; done < frames;) {         // pull in 1024-frame blocks, ticking the streams like the game loop
        int n = frames - done < 1024 ? frames - done : 1024;
        tb_dsound_mix(&mix[(size_t)done * 2], n);
        Vision::Sound::tickAll();
        done += n;
    }
    write_wav(out + "/" + name + ".mode" + std::to_string(mode) + ".wav", mix, 44100, 2);
    // compare the mix against the plain decode (44100 mono sources are copied 1:1 to both channels)
    if (w.rate == 44100) {
        size_t n = pcm.size() < (size_t)frames ? pcm.size() : (size_t)frames, bad = 0, first = (size_t)-1;
        for (size_t i = 0; i < n; i++)
            if (mix[i * 2] != pcm[i]) { if (first == (size_t)-1) first = i; bad++; }
        tb_log("test: mode %d vs decode: %zu of %zu samples differ%s", mode, bad, n,
               bad ? (" (first at " + std::to_string(first) + ")").c_str() : "");
    }
    tb_log("test: wrote %zu mixed frames", mix.size() / 2);
    s.close();
    return 0;
}

static int test_movie(const char *name) {
    const char *e;
    std::string out = (e = getenv("TB_TEST_OUT")) ? e : ".";
    tb_dsound_create();                         // SDL audio (SDL_AUDIODRIVER=dummy is fine)
    Vision::QTMovie mv(name);
    mv.open();
    if (!mv.loaded()) return 2;
    int w, h;
    mv.size(&w, &h);
    mv.play();
    uint32_t t0 = SDL_GetTicks();
    int shots = 0, frames = 0;
    double next_shot[4] = { 0.5, 2.0, 5.0, 9.0 };
    while (mv.isPlaying() && SDL_GetTicks() - t0 < 60000) {
        if (mv.task()) frames++;
        double t = (SDL_GetTicks() - t0) / 1000.0;
        if (shots < 4 && t >= next_shot[shots] && mv.frame()) {
            SDL_Surface *sf = SDL_CreateRGBSurfaceFrom((void *)mv.frame(), w, h, 16, w * 2, 0xF800, 0x07E0, 0x001F, 0);
            std::string p = out + "/" + name + "_" + std::to_string(shots) + ".bmp";
            SDL_SaveBMP(sf, p.c_str());
            SDL_FreeSurface(sf);
            tb_log("test: saved %s at %.2fs", p.c_str(), t);
            shots++;
        }
        SDL_Delay(5);
    }
    tb_log("test: movie ended after %.2fs, %d frames shown (%dx%d)", (SDL_GetTicks() - t0) / 1000.0, frames, w, h);
    mv.dispose();
    tb_dsound_destroy();
    return 0;
}

// Called by main() in core.cpp before tb_game_main (weak there); -1 = nothing to test, run the game.
int tb_test_hook(int, char **) {
    const char *s = getenv("TB_TEST_SOUND"), *m = getenv("TB_TEST_MOVIE");
    if (s && *s) return test_sound(s);
    if (m && *m) return test_movie(m);
    return -1;
}
