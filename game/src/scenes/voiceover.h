// The voice-over classes (lane B), reconstructed from "Tinker Bell.exe":
//   VoiceOver       0x4c8ca0 ctor, 0x4c8f70 dtor, 0x4c9400 play, 0x4c9260 stop, 0x4c9690 isPlaying,
//                   0x4c96d0 draw (lip sync), 0x4c97f0 load, 0x4c9840 unload, 0x4c9890 Space stops, 0x4c91b0 registerLine,
//                   0x4c8850 the MagPies.dat loader. In the exe one VoiceOver is a member of the game state (+0x3e8):
//                   every caller does `fcn.004c9400(state + 0x3e8, "<line>")`. Port: VoiceOver::get().
//   MagpieCharacter 0x514244: the abstract speaker (Wendy, Peter...) a voice line is registered with.
//   Acts/tasks that play lines (voacts.h): PlayVos, DelayedVO, PreSceneVo, VoOneAfterAnother, QuestionRepeater, Reminder.
//
// Interface for the game scenes (lane A):
//   VoiceOver &vo = VoiceOver::get();         // the one voice-over object (created on first use)
//   vo.load();                                // once, when the game starts (activates its Screen, task, Space key)
//   vo.registerLine("SIGN_VO_WENwall", &wendy);   // optional: which MagpieCharacter animates this line
//   vo.play("SIGN_VO_WENwall");               // 0x4c9400: stops the previous line, streams audio/<line>.wv6
//   vo.isPlaying("SIGN_VO_WENwall");          // 0x4c9690: this line is the current one and has not finished
//   vo.stop();                                // 0x4c9260 (a line ending in "_y" chains to the "_z" line)
// While a line plays, VoiceOver::draw() (once per frame, from its own Screen) looks up the line's mouth-shape
// letter for the elapsed time (one per 100 ms, from MagPies.dat) and calls character->mouth(letter).
#pragma once
#include "../vision/engine.h"
#include "../vision/sound.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Vision {

// 0x514244: dtor + three pure virtuals (the vtable slots +4, +8, +0xc that VoiceOver calls).
class MagpieCharacter {
public:
    virtual ~MagpieCharacter() {}               // 0x4437e0
    virtual bool ready() = 0;                   // +4: the character can start speaking (checked before the audio starts)
    virtual void mouth(int letter) = 0;         // +8: mouth shape 0..10 ('a'..'k' of MagPies.dat)
    virtual void rest() = 0;                    // +0xc: the line ended; back to the neutral mouth
};

// A client of the voice-over keyboard service: Space stops the line (0x4c9890, the sub-object at VoiceOver+0x98).
class VoiceOver;
class VoKeyClient : public Task {
public:
    explicit VoKeyClient(VoiceOver *vo) : Task(nullptr), vo(vo) {}
    void notify2(Service *s) override;          // 0x4c9890
    VoiceOver *vo;
};

class VoiceOver : public Graphic {              // vtable 0x5172a8 (Graphic + a Task sub-object at +0x98)
public:
    VoiceOver();                                // 0x4c8ca0
    ~VoiceOver() override;                      // 0x4c8f70
    static VoiceOver &get();                    // port: the game state's member (+0x3e8)

    void draw() override;                       // +0x38 0x4c96d0: starts the audio and follows the lip sync
    void load() override;                       // +0x3c 0x4c97f0
    void unload() override;                     // +0x40 0x4c9840
    int canvasMode() override { return 1; }     // +0x10 0x4aada0

    void play(const char *name);                // 0x4c9400
    void stop();                                // 0x4c9260
    bool isPlaying(const char *name) const;     // 0x4c9690: `name` is the current line and it has not ended
    bool busy() const { return playingFlag; }   // +0x178
    void registerLine(const char *name, MagpieCharacter *c);   // 0x4c91b0: lines_[name] = c
    const std::string &current() const { return cur; }         // +0x15c

    // the MagPies.dat table (0x4c8850): line -> mouth letters (0..10, one per 100 ms)
    const std::map<std::string, std::vector<int8_t>> &lips() const { return lips_; }

    // port: 0x4c9260 asks the game state (0x4e9d70(state+0x3cc, *0x543528)) whether "FLYH_VO_RAC1more_y" must not chain
    // to its "_z" line. Lane A sets this; default: false (chain).
    std::function<bool()> skipChainFlyhMore;
    // port: the flag the tasks test before repeating a line (*(state+0x560) != 0 suppresses them). Lane A sets this.
    std::function<bool()> suppressed;

    bool spaceStops = true;                     // +0x264 (sub-object +0x1cc)

private:
    void loadLips();                            // 0x4c8850
    friend class VoKeyClient;
    Sound *sound = nullptr;                     // +0x158
    std::string cur;                            // +0x15c
    bool playingFlag = false;                   // +0x178
    std::map<std::string, MagpieCharacter *> lines_;   // +0x168 (M1)
    std::string speaker;                        // +0x17c: the line whose character animates (empty = none)
    uint32_t startTick = 0;                     // +0x188
    std::map<std::string, std::vector<int8_t>> lips_;  // +0x240 (M2)
    const std::vector<int8_t> *lipsCur = nullptr;      // +0x250
    Screen screen;                              // +0x18c
    Keyboard kbd;                               // +0x208
    VoKeyClient client;                         // +0x98
};

// Convenience used by every task below: `fcn.004c9400(*(0x63171c) + 0x3e8, name)`.
inline void vo_play(const char *name) { VoiceOver::get().play(name); }
inline bool vo_isPlaying(const char *name) { return VoiceOver::get().isPlaying(name); }

}  // namespace Vision
