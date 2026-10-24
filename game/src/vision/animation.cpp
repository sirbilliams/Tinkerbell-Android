// Vision::Animation, AnimationHelper, AnimationPlayer (lane A). See animation.h.
#include "animation.h"
#include "tb.h"
#include <stdio.h>
#include <math.h>

namespace Vision {

// 0x412660 Graphic::v5: the size follows the scale from the original size; 100 restores it.
double Graphic::setScale(double percent) {
    if (percent == 100.0) {
        h = oh; w = ow;
        dirty();
        scale = 100.0;
        return 100.0;
    }
    double k = percent * 0.01;               // 0x513560
    w = (int)lround((double)ow * k);         // 0x425b50 rounds
    h = (int)lround((double)oh * k);
    dirty();
    scale = percent;
    return percent;
}

// 0x404c20 Animation ctor: a hidden-size Graphic with the default frame rate, direction 1, not started.
Animation::Animation(const char *p) : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {
    fps = tb_exe_i32(0x53282c);
    if (p) { prefix = p; setName(p); }
}
Animation::~Animation() { unload(); }        // 0x404d20: clears the list, then the Graphic

void Animation::unload() {                   // 0x404380
    for (Image *f : frames) delete f;
    frames.clear();
}

// the probe the exe makes with 0x41a3d0 + release 0x4193c0
static bool sprite_exists(const char *n) {
    Resource *r = ras_find(n, "sp5");
    if (!r) return false;
    r->release();
    return true;
}

// 0x404460 v15
void Animation::load() {
    off_x = off_y = 0;
    unload();
    auto add = [&](const char *name) {       // new Image(name, 0,0,0,0,0,0,visible), loaded, hit/translucency/f90 inherited
        Image *im = new Image(name, 0, 0, 0, 0, 0, 0, true);
        im->load();
        im->setHit(hitmode, false);          // port: the exe calls +0x18 with this+0x50 and 0
        im->translucency = translucency;
        im->f90 = f90;
        frames.push_back(im);
    };
    if (names.empty()) {
        char buf[256];
        int width = 2;
        snprintf(buf, sizeof buf, "%s01", prefix.c_str());
        if (!sprite_exists(buf)) {
            width = 3;
            snprintf(buf, sizeof buf, "%s001", prefix.c_str());
            if (!sprite_exists(buf)) {
                width = 1;
                snprintf(buf, sizeof buf, "%s1", prefix.c_str());
                if (!sprite_exists(buf)) {
                    if (!sprite_exists(prefix.c_str())) {
                        vision_log("Animation %s does not exist.", prefix.c_str());
                        return;
                    }
                    add(prefix.c_str());     // a single sprite named exactly the prefix
                    goto done;
                }
            }
        }
        do {
            add(buf);
            snprintf(buf, sizeof buf, "%s%*.*d", prefix.c_str(), width, width, (int)frames.size() + 1);
        } while (sprite_exists(buf));
    } else {
        for (const std::string &n : names) add(n.c_str());
    }
done:
    // the Animation takes the first frame's box (x y z w h and the original size)
    if (frames.empty()) return;
    Image *f0 = frames[0];
    x = f0->x; y = f0->y; z = f0->z; w = f0->w; h = f0->h;
    ow = f0->w; oh = f0->h;
    setFrame(0);
}

Image *Animation::frame(int n) {             // 0x403ec0
    if (frames.empty()) return nullptr;
    if (n < 0) n = frame_no;
    n %= (int)frames.size();
    return frames[n];
}

int Animation::setFrame(int n) {             // 0x403f20
    if (frames.empty()) return -1;
    frame_no = n % (int)frames.size();
    Image *f = frames[frame_no];
    off_x = f->x - frames[0]->x;
    off_y = f->y - frames[0]->y;
    f->setScale(scale);
    w = f->w; h = f->h;
    return frame_no;
}

double Animation::setScale(double percent) { // 0x4043a0 v5
    Graphic::setScale(percent);
    if (!frames.empty()) setFrame(frame_no);
    return scale;
}

void Animation::setHit(int m, bool f) {      // 0x403cf0 v6
    f90 = f;
    hitmode = m;
    for (Image *im : frames) im->setHit(m, f);
}

void Animation::place(Image *f, int &sx, int &sy) {
    sx = f->x; sy = f->y;
    if (scale == 100.0) {
        f->x = off_x + x;
        f->y = off_y + y;
    } else {
        // port: unsure. The exe subtracts a float->int (0x4f3948) from the animation's x/y; the scaled offset
        // of the frame from the first frame is the natural reading.
        f->x = x + (int)(off_x * scale * 0.01);
        f->y = y + (int)(off_y * scale * 0.01);
    }
}

bool Animation::hit(int px, int py) {        // 0x403c20 v11
    if (frames.empty()) return false;
    Image *f = frame();
    int sx, sy;
    place(f, sx, sy);
    bool r = f->hit(px, py);
    f->x = sx; f->y = sy;
    return r;
}

bool Animation::step() {                     // 0x403d60
    int n = (int)frames.size();
    if (n == 0) return false;
    if (mode == 0) {
        if (dir < 0 && frame_no == 0) return false;
        if (dir > 0 && frame_no == n - 1) return false;
    }
    bool ok = true;
    int i = frame_no + dir;
    if (i < 0) {
        if (mode == 0) { i = 0; ok = false; }
        else if (mode == 1) { i = n - 1; ok = true; }
        else if (mode == 2) { i = -dir; dir = i; }
    } else if (i >= n) {
        if (mode == 0) { i = n - 1; ok = false; }
        else if (mode == 1) { i = 0; ok = true; }
        else if (mode == 2) { dir = -dir; ok = true; i = dir - 1 + n; }
    }
    if (i < 0) i = 0;
    setFrame(i);
    return ok;
}

void Animation::draw() {                     // 0x403fe0 v14
    int n = (int)frames.size();
    if (n == 0) return;
    if (start_time != 0 && fps > 0) {
        uint32_t per = (uint32_t)(1000 / fps);
        if (per == 0) per = 1;
        int e = (int)((real_time() - start_time) / per);
        int idx = -1;
        if (mode == 0) {
            if (dir > 0) idx = e < n - 1 ? e : n - 1;
            else if (dir < 0) { idx = n - e - 1; if (idx < 0) idx = 0; }
        } else if (mode == 1) {
            if (dir > 0) idx = e % n;
            else if (dir < 0) idx = n - (e % n) - 1;
        } else if (mode == 2 && e < n * 2) {
            bool even = ((e / n) & 1) == 0;
            int r = e % n;
            if (dir > 0) idx = even ? r : n - r - 1;
            else if (dir < 0) idx = even ? n - r - 1 : r;
        }
        if (idx >= 0) setFrame(idx);
    }
    Image *f = frame();
    int sx, sy;
    place(f, sx, sy);
    f->draw();
    f->x = sx; f->y = sy;
}

bool Animation::playing() {                  // 0x4e3110
    if (frames.empty() || start_time == 0 || fps <= 0) return false;
    int per = 1000 / fps;
    int e = per ? (int)((real_time() - start_time) / (uint32_t)per) : 0;
    if (mode == 0 && (int)frames.size() <= e) return false;
    if (mode == 2 && (int)frames.size() * 2 <= e) return false;
    return true;
}
void Animation::start() {                    // 0x4e3190
    setFrame(0);
    start_time = real_time();
}
void Animation::forward() {                  // 0x4e31b0: restart the clock so that the frame on screen stays
    if (dir == 1 || fps <= 0) return;
    uint32_t per = (uint32_t)(1000 / fps);
    uint32_t el = real_time() - start_time;
    if (el < per) el = per;
    dir = 1;
    start_time = (el - (uint32_t)count() * per) + real_time();
}
void Animation::reverse() {                  // 0x4e3220
    if (dir == -1 || fps <= 0) return;
    uint32_t per = (uint32_t)(1000 / fps);
    uint32_t el = real_time() - start_time;
    if (el < per) el = per;
    dir = -1;
    start_time = (el - (uint32_t)count() * per) + real_time();
}

AnimationHelper::AnimationHelper(const char *p, int m) : Animation(p) {   // 0x4e3040
    mode = m;
    load();
}

// ---------------------------------------------------------------- AnimationPlayer
AnimationPlayer::AnimationPlayer()           // 0x4e32b0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), anim("") {}
AnimationPlayer::~AnimationPlayer() {}       // 0x4e3370
void AnimationPlayer::load() { playing_ = false; anim.load(); }        // 0x4e33f0
void AnimationPlayer::unload() { Graphic::unload(); anim.unload(); }   // 0x4e3410
void AnimationPlayer::start(bool loop) {     // 0x4e34c0
    last_tick = real_time();
    anim.setFrame(0);
    playing_ = true;
    anim.mode = loop ? 1 : 0;                // port: the exe stores the argument in the animation's +0x104
}
void AnimationPlayer::draw() {               // 0x4e3430
    if (!playing_) return;
    uint32_t now = real_time();
    if (delay <= now - last_tick) {
        last_tick = now;
        if (anim.mode == 0 && anim.count() - 1 <= anim.current()) { playing_ = false; return; }
        anim.step();
    }
    anim.x = x; anim.y = y;
    anim.draw();
}

}  // namespace Vision
