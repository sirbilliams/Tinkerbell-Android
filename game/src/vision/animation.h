// The Vision Animation (vtable 0x5134cc: Graphic + a list of Image frames), AnimationHelper (0x5177f0: the
// Animation(prefix, mode) the scenes construct with 0x4e3040) and AnimationPlayer (0x517840) (lane A).
// Frames are sprites named <prefix>01, <prefix>02, ... (or 001.., or 1.., or the bare prefix as one frame).
// Play modes (+0x104): 0 = once, 1 = loop, 2 = ping-pong. Frame rate (+0x108) is frames per second.
#pragma once
#include "engine.h"

namespace Vision {

class Animation : public Graphic {
public:
    explicit Animation(const char *prefix = "");     // 0x404c20
    ~Animation() override;                           // 0x404d20
    // 0x404460 v15: finds the frames (explicit names in `names`, else prefix01/001/1/bare) and loads them
    void load() override;
    void unload() override;                          // 0x404380 v16: drops the frames
    double setScale(double percent) override;        // 0x4043a0 v5
    void setHit(int mode, bool f90) override;        // 0x403cf0 v6: on every frame
    bool hit(int x, int y) override;                 // 0x403c20 v11
    void draw() override;                            // 0x403fe0 v14: picks the frame from the clock when started, draws it

    int count() const { return (int)frames.size(); }               // 0x403c10
    int current() const { return frames.empty() ? -1 : frame_no; } // 0x403f00
    Image *frame(int n = -1);                        // 0x403ec0 (n < 0 = the current one)
    int setFrame(int n);                             // 0x403f20: shows frame n % count, returns it (-1 when empty)
    bool step();                                     // 0x403d60: next frame by `dir` per the mode; false at the end of a once-run
    // AnimationHelper's calls (0x4e3110/0x4e3190/0x4e31b0/0x4e3220), here on the base for use by both
    bool playing();                                  // 0x4e3110: started and not past the end
    void start();                                    // 0x4e3190: frame 0 and the clock
    void forward();                                  // 0x4e31b0: dir = 1 keeping the frame
    void reverse();                                  // 0x4e3220: dir = -1 keeping the frame
    void stop() { start_time = 0; }                  // port: a clean stop (the exe sets +0x10c = 0 where it needs one)

    std::string prefix;                              // the name the frames are found by
    std::vector<std::string> names;                  // +0xf8/+0xfc: explicit frame names (used first when non-empty)
    std::vector<Image *> frames;                     // +0xc8/+0xcc (the exe holds Nodes; +8 = the Image)
    int frame_no = 0;                                // +0x98
    int off_x = 0, off_y = 0;                        // +0x9c +0xa0: the current frame's offset from frame 0
    int mode = 0;                                    // +0x104: 0 once, 1 loop, 2 ping-pong
    int fps;                                         // +0x108 (default: the exe's 0x53282c)
    uint32_t start_time = 0;                         // +0x10c: 0 = not started (shows frame_no as it is)
    int dir = 1;                                     // +0x110
private:
    void place(Image *f, int &sx, int &sy);          // moves the frame to the animation's spot (draw / hit), saves its own
};

class AnimationHelper : public Animation {           // 0x4e3040(prefix, mode)
public:
    AnimationHelper(const char *prefix, int mode);
};

// 0x4e32b0: a Graphic that owns an Animation and steps it by its own delay.
class AnimationPlayer : public Graphic {
public:
    explicit AnimationPlayer(const char *prefix = "", uint32_t delay = 0);   // 0x4e32b0
    ~AnimationPlayer() override;                     // 0x4e3370
    void load() override;                            // 0x4e33f0 v15: stops, loads the animation
    void unload() override;                          // 0x4e3410 v16
    void draw() override;                            // 0x4e3430 v14: every `delay` ms steps the frame, then draws
    void start(bool loop);                           // 0x4e34c0
    bool playing_ = false;                           // +0x98
    uint32_t last_tick = 0;                          // +0x9c
    uint32_t delay = 0;                              // +0xa0 (ms)
    Animation anim;                                  // +0xa8
};

}  // namespace Vision
