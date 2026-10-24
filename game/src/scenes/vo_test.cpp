// Lane B's desktop test for the voice-over classes: `TB_TEST_VO=<voice line> tinkerbell votest`. Not part of the game.
// Plays the line with a logging MagpieCharacter and reports the length and the lip-sync letters per frame.
#include "voacts.h"
#include <stdlib.h>
#include <stdio.h>

namespace Vision {
namespace {
struct LogChar : MagpieCharacter {
    uint32_t t0 = 0, frames = 0, changes = 0;
    int last = -2;
    std::string seq;
    bool ready() override { return true; }
    void mouth(int c) override {
        if (!t0) t0 = real_time();
        frames++;
        if (c != last) { changes++; seq += (char)('a' + c); last = c; }
        if (frames % 10 == 1) vision_log("VOTEST t=%u mouth=%c", real_time() - t0, 'a' + c);
    }
    void rest() override { vision_log("VOTEST rest after %u ms, %u mouth calls, %u changes", real_time() - t0, frames, changes); }
};
struct VoTestScene : Scene {
    LogChar ch;
    std::string line;
    uint32_t t0 = 0, ticks = 0;
    VoTestScene(const char *l) : Scene("votest"), line(l) {}
    void enter() override {
        VoiceOver &vo = VoiceOver::get();
        vo.load();
        vo.registerLine(line.c_str(), &ch);
        auto it = vo.lips().find(line);
        vision_log("VOTEST line=%s lipsync letters=%d (=%d ms)", line.c_str(), it == vo.lips().end() ? -1 : (int)it->second.size(),
                   it == vo.lips().end() ? 0 : (int)it->second.size() * 100);
        vo.play(line.c_str());
        t0 = real_time();
        activate();
    }
    void run() override {
        for (;;) {
            wait(500);
            VoiceOver &vo = VoiceOver::get();
            if (!vo.isPlaying(line.c_str())) {
                vision_log("VOTEST done: line ended after %u ms; mouth letters seen: %s", real_time() - t0, ch.seq.c_str());
                if (getenv("TB_TEST_VO_ACTS")) {         // DelayedVO then VoOneAfterAnother (the same line twice)
                    uint32_t before = ch.frames;
                    DelayedVO d(line.c_str(), 300);
                    d.start();
                    wait(4500);
                    vision_log("VOTEST DelayedVO: %s (%u new mouth calls), now stopped=%d", ch.frames > before ? "played" : "DID NOT PLAY",
                               ch.frames - before, (int)!d.active());
                    before = ch.frames;
                    VoOneAfterAnother o;
                    o.add(line.c_str());
                    o.add(line.c_str());
                    o.activate();
                    wait(7000);
                    vision_log("VOTEST VoOneAfterAnother: %u new mouth calls, index=%u, active=%d", ch.frames - before, o.index, (int)o.active());
                    d.stop();
                }
                for (;;) wait(1000);
            }
        }
    }
};
struct Init { Init() { if (const char *l = getenv("TB_TEST_VO")) new VoTestScene(l); } } g_init;
}  // namespace
}  // namespace Vision
