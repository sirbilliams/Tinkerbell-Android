// The tasks that play voice-over lines (lane B), reconstructed from "Tinker Bell.exe". All talk to VoiceOver::get().
//   VoList            the cycling list of lines of PlayVos            0x4cc080 ctor, 0x4cc0d0/0x4cc1a0 add, 0x4cc0e0 playNext, 0x4cc130 anyPlaying
//   PlayVos           vtable 0x5149f0  (member of the shell game)      run 0x4c5c80
//   DelayedVO         vtable 0x51694c  (Act; 5 in the sign-in room)    ctor 0x4c1530, start 0x4c15f0, stop 0x4c1630, notify2 0x4c1670, run 0x4c16c0
//   PreSceneVo        vtable 0x515fd8  (treasure map)                  run 0x4c5dc0
//   VoOneAfterAnother vtable 0x515948  (movie endings)                 add 0x4cba80, run 0x4cbd90
//   QuestionRepeater  vtable 0x5171d4                                  ctor 0x4c5f00, start 0x4c5fa0, stop 0x4c60f0, notify2 0x4c60c0, run 0x4c6000
//   Reminder          vtable 0x517208                                  ctor 0x4c6140, start 0x4c6220, stop 0x4c6260, run 0x4c6290
// Use: construct, then start(): DelayedVO::start(), QuestionRepeater::start(line), Reminder::start(); the others with activate().
#pragma once
#include "voiceover.h"
#include "../vision/act.h"

namespace Vision {

void set_cursor(Graphic *g, bool visible);   // 0x4119b0
void show_cursor();                          // 0x4119d0
void hide_cursor();                          // 0x4119e0
extern Graphic *g_cursor_graphic;       // 0x5a69c4: the mouse cursor (lane A sets it); PreSceneVo hides it
extern bool (*g_vo_suppressed)();       // *(0x63171c + 0x560) != 0: the repeaters stay quiet (lane A sets it; default: never)

class VoList {                          // the lines PlayVos cycles through
public:
    std::vector<std::string> lines;     // +0x24..+0x2c
    int cursor = 0;                     // +0x30
    void add(const char *line) { lines.push_back(line); }   // 0x4cc1a0
    void playNext();                    // 0x4cc0e0: plays lines[cursor], then cursor+1 (wrapping)
    bool anyPlaying() const;            // 0x4cc130
};

class PlayVos : public Task {
public:
    explicit PlayVos(VoList *list, uint32_t period = 12000) : Task(nullptr), list(list), period(period) {}
    void run() override;                // 0x4c5c80: every `period` ms plays the next line of the list
    VoList *list;                       // +0xc0
    uint32_t next = 0;                  // +0xc4
    uint32_t period;                    // +0xc8 (12000 in the shell game)
};

class DelayedVO : public Act {
public:
    DelayedVO(const char *line, uint32_t delay);   // 0x4c1530
    ~DelayedVO() override {}
    void start() override;              // +0x30 0x4c15f0: arms the timer and the mouse/keyboard watchers
    void stop() override;               // +0x34 0x4c1630
    void notify2(Service *s) override;  // +0x24 0x4c1670: a click or a key press restarts the delay
    void run() override;                // 0x4c16c0: after `delay` ms without input plays the line, then stops
    uint32_t last = 0;                  // +0xc0
    Mouse mouse;                        // +0xc4
    Keyboard kbd;                       // +0xf8
    uint32_t delay;                     // +0x12c
    std::string line;                   // +0x130 (a char* in the exe)
};

class PreSceneVo : public Task {
public:
    PreSceneVo(const char *line, const char *scene) : Task(nullptr), line(line), scene(scene) {}
    void run() override;                // 0x4c5dc0: hides the cursor, plays the line, then changes scene
    std::string line;                   // +0xc0
    std::string scene;                  // +0xcc
};

class VoOneAfterAnother : public Task {
public:
    VoOneAfterAnother() : Task(nullptr) {}
    void add(const char *line) { lines.push_back(line); }   // 0x4cba80
    void run() override;                // 0x4cbd90: plays the lines one after the other (checks every 100 ms)
    std::vector<std::string> lines;     // +0xc4
    uint32_t index = 0;                 // +0xd0
};

class QuestionRepeater : public Task {
public:
    QuestionRepeater();                 // 0x4c5f00
    void start(const char *line);       // 0x4c5fa0: repeats `line` every 15 s of silence
    void stop();                        // 0x4c60f0
    void notify2(Service *s) override;  // 0x4c60c0: a click restarts the 15 s
    void run() override;                // 0x4c6000
    Mouse mouse;                        // +0xc0
    std::string line;                   // +0xf4
    uint32_t next = 0;                  // +0x100
};

class Reminder : public Task {
public:
    Reminder(const char *image, const char *line);  // 0x4c6140
    void start();                       // 0x4c6220: shows the image, blocks input to everything else (0x5a747c)
    void stop();                        // 0x4c6260
    void run() override;                // 0x4c6290: plays the line, then hides the image and finishes
    Image img;                          // +0xc0
    std::string line;                   // +0x168
    Screen screen;                      // +0x174
};

}  // namespace Vision
