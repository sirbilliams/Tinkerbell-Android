// The 3D scenes' corner buttons, reconstructed from "Tinker Bell.exe" (see toolbar.h).
#include "toolbar.h"
#include "state.h"
#include "voacts.h"
#include "../vision/engine.h"
#include <float.h>

namespace Game {

using Raytrace::Sprite3D;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30

// ---------------------------------------------------------------- HelpBellIcon
HelpBellIcon::HelpBellIcon()                         // 0x4e6df0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      idle("GLOB_BU_BELidl_01", 0, 0, 0, 0, 0, 0, true), rov("GLOB_BU_BELrov_01", 0, 0, 0, 0, 0, 0, true),
      sel("GLOB_BU_BELsel_"), ring("GLOB_FX_BELring") {}

HelpBellIcon::~HelpBellIcon() {}                     // 0x4e6f00

bool HelpBellIcon::hit(int px, int py) {             // v11 0x4e7210
    return idle.hit(px, py) || rov.hit(px, py);
}

// v14 0x4e6ff0: the ring animation while it runs, else the idle or the roll-over picture.
void HelpBellIcon::draw() {
    if (sel.start_time != 0) {
        uint32_t per = (uint32_t)(1000 / sel.fps);
        uint32_t n = (frame_time() - sel.start_time) / per;
        if ((int)n < sel.count()) { sel.draw(); return; }
    }
    if (idle.visible) { idle.draw(); return; }
    if (rov.visible) rov.draw();
}

void HelpBellIcon::load() {                          // v15 0x4e7080
    idle.load();
    idle.visible = true;
    idle.setHit(1, true);
    rov.load();
    rov.visible = false;
    sel.load();
    sel.visible = false;
    z = 10;                                          // 0x411e70(10)
    if (layer) layer->sort();
    ring.type = 7;                                   // +0x640
    ring.open();
    pause.set(this, "GLOB_VO_PETbellpo", 1000);      // 0x4c5970
    pause.start();                                   // +0x30
}

void HelpBellIcon::unload() {                        // v16 0x4e7120
    Graphic::unload();
    pause.stop();                                    // +0x34
    idle.unload();
    rov.unload();
    sel.unload();
    ring.close();                                    // 0x41fc50
}

// 0x4e7180: over it the roll-over shows; a click rings it (true).
bool HelpBellIcon::click() {
    if (!visible) return false;
    idle.visible = true;
    rov.visible = false;
    if (!idle.hit(g_mouse.x, g_mouse.y)) return false;
    idle.visible = false;
    rov.visible = true;
    if (!mouse_down()) {
        rov.visible = true;
        return false;
    }
    ring.play();
    sel.setFrame(0);
    sel.start_time = frame_time();                   // 0x403be0(0, 0)
    sel.mode = 0;
    sel.visible = true;
    return true;
}

// ---------------------------------------------------------------- NeverlandMapIcon
NeverlandMapIcon::NeverlandMapIcon()                 // 0x4ef8e0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      idle("GLOB_BU_MAPidl_01", 0, 0, 0, 0, 0, 0, true), rov("GLOB_BU_MAProv_01", 0, 0, 0, 0, 0, 0, true),
      vo("DEMO_VO_PETmap1") {
    pause.set(this, "GLOB_VO_PETmap", 1000);         // 0x4c5970
}

NeverlandMapIcon::~NeverlandMapIcon() {}

// v11 0x4efba0: only while it and the cursor show.
bool NeverlandMapIcon::hit(int px, int py) {
    if (!visible || !g_cursor_graphic || !g_cursor_graphic->visible) return false;
    return idle.hit(px, py);
}

// v14 0x4efbe0: lit while its line plays (and for 2 s after), or under the mouse without dust on the cursor.
void NeverlandMapIcon::draw() {
    uint32_t dt = frame_time() - last;
    if (vo.isPlaying()) pause.cancel();              // 0x4c5be0
    if (dt < vo.lengthMs() + 2000) { rov.draw(); return; }
    if (hit(g_mouse.x, g_mouse.y) && !state().dusting) { rov.draw(); return; }
    idle.draw();
}

void NeverlandMapIcon::load() {                      // v15 0x4efae0
    idle.load();
    rov.load();
    vo.mode = 1;                                     // +0x520
    vo.close();
    vo.open();
    last = (uint32_t)(-2000 - (int32_t)vo.lengthMs()) + frame_time();
    pause.start();                                   // +0x30
}

void NeverlandMapIcon::unload() {                    // v16 0x4efb50
    Graphic::unload();
    pause.stop();
    idle.unload();
    rov.unload();
    vo.close();
}

int NeverlandMapIcon::click(int px, int py, bool play) {   // 0x4efc90
    if (!hit(px, py)) return 0;
    if (vo.isPlaying()) return 2;
    uint32_t now = frame_time();
    if (now - last < vo.lengthMs() + 2000) return 2;
    last = now;
    if (!play) return 2;
    VoiceOver::get().stop();
    vo.play();
    pause.cancel();
    return 1;
}

// ---------------------------------------------------------------- PixieDustBag
static const char *const kBagLevels[5] = {"glob_bu_bagidl01_01", "glob_bu_bagidl02_01", "glob_bu_bagidl03_01",
                                          "glob_bu_bagidl04_01", "glob_bu_bagidl05_01"};

PixieDustBag::PixieDustBag()                         // 0x4efd50
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      puff("glob_bu_bagpffsel_") {
    for (int i = 0; i < 5; i++) level[i] = new Image(kBagLevels[i], 0, 0, 0, 0, 0, 0, true);   // 0x453740 + 0x424400
    dustin = new AnimationHelper("glob_if_dustin_", 0);   // 0x4e3040
    pause.set(this, "GLOB_VO_PETdstbag", 1000);
    pause.start();
    for (Image *i : level) i->load();                // v15
    puff.load();
    regen_time = frame_time();
    puff_time = frame_time();
    puff.visible = false;                            // +0x464
    State &st = state();
    dust = &st.dust;                                 // +0x3c8
    if (!st.saves.getDust(dust)) {                   // 0x4ea760
        *dust = 50;
        st.saves.setDust(50);                        // 0x4ea7b0
        st.saves.flush();                            // 0x4e9840
    }
    if (*dust > 50) *dust = 50;
    x = -220;                                        // +0x58
    y = 209;                                         // +0x5c
    dustin->x = -220;
    int mh = 0;
    for (int i = 0; i < dustin->count(); i++)
        if (dustin->frame(i)->h > mh) mh = dustin->frame(i)->h;   // 0x403ec0, +0x40
    dustin->y = y - (level[0]->h + mh) / 2;          // +0xd8: the first level's height
}

PixieDustBag::~PixieDustBag() {                      // 0x4f0000
    pause.stop();                                    // +0x34
    for (Image *&i : level) { i->unload(); delete i; i = nullptr; }
    delete dustin;
    dustin = nullptr;
    puff.unload();
    state().saves.setDust(*dust);
    state().saves.flush();
}

int PixieDustBag::levelIndex() const {
    int i = (*dust + 4) / 10;
    return i > 4 ? 4 : i;
}

void PixieDustBag::add(int n) {                      // 0x4efd30
    *dust += n;
    if (*dust < 0) *dust = 0;
    if (*dust > 50) *dust = 50;
}

bool PixieDustBag::hit(int px, int py) {             // v11 0x4f0170
    if (!visible) return false;
    return level[levelIndex()]->hit(px, py);
}

// v14 0x4f01d0: the bag at its level, the dust going in, the puff (a frame every 60 ms).
void PixieDustBag::draw() {
    Image &l = *level[levelIndex()];
    l.x = x;
    l.y = y;
    l.draw();
    if (dustin->visible) dustin->draw();
    if (dustin->visible && !dustin->playing()) dustin->visible = false;   // 0x4e3110
    if (puff.visible) {
        puff.x = x;
        puff.y = y;
        puff.draw();
    }
    if (frame_time() - puff_time > 60 && puff.visible) {
        if (puff.current() >= puff.count() - 1) { puff.visible = false; return; }
        puff.step();                                 // 0x403d60
    }
}

static int by_skill(int a, int b, int c, int other) {
    switch (state().skill) {
    case 1: return a;
    case 2: return b;
    case 3: return c;
    default: return other;
    }
}

// 0x4f02f0: a click on the bag. With dust on the cursor: put back (refunded); else take some if there is enough
// (the cost goes, the puff plays), or Peter says to fill it.
bool PixieDustBag::click() {
    if (!mouse_down() || !hit(g_mouse.x, g_mouse.y)) return false;
    State &st = state();
    if (st.dusting) {
        st.dusting = false;
        switch (st.skill) {
        case 1: dustin->visible = true; dustin->start(); add(2); break;
        case 2: dustin->visible = true; dustin->start(); add(3); break;
        case 3: dustin->visible = true; dustin->start(); add(4); break;
        default: break;
        }
        return true;
    }
    int c = by_skill(2, 3, 4, 0);
    if (*dust < c) {
        VoiceOver::get().play("GLOB_VO_PETfilbag");
        return true;
    }
    st.dusting = true;
    puff.visible = true;
    puff.setFrame(0);
    add(-by_skill(2, 3, 4, 0));
    return true;
}

void PixieDustBag::sprinkle(Raytrace::RayTrace *rt, Sprite3D **sprite, Raytrace::Attachment **att) {   // 0x4f0480
    State &st = state();
    if (!st.dusting || !mouse_down()) return;
    float keep = rt->pick_max;                       // +0x1170
    rt->pick_max = FLT_MAX;                          // 0x7f7fffff
    *sprite = rt->pick(g_mouse.x, g_mouse.y);        // 0x42c710
    // port: 0x42c950 (the attachment under the mouse, any distance) is not ported yet; no scene so far uses it
    *att = nullptr;
    rt->pick_max = keep;
    if (st.dusting) {
        st.throwdust->close();                       // +0x7c
        st.throwdust->open();
        st.throwdust->play();
        st.dusting = false;
    }
    set_cursor(st.cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);   // 0x4119b0(+0x5c)
}

void PixieDustBag::refillSmall() {                   // 0x4f0570
    dustin->visible = true;
    dustin->start();
    switch (state().skill) {
    case 1: add(15); break;
    case 2: add(12); break;
    case 3: add(10); break;
    default: break;
    }
}

void PixieDustBag::refillLarge() {                   // 0x4f05f0
    dustin->visible = true;
    dustin->start();
    switch (state().skill) {
    case 1: add(30); break;
    case 2: add(24); break;
    case 3: add(20); break;
    default: break;
    }
}

void PixieDustBag::refillRandom() {                  // 0x4f0670
    int r = vision_random(5);                        // 0x425d10 + 0x425d40
    switch (state().skill) {
    case 1: dustin->visible = true; dustin->start(); add(r + 4); break;
    case 2: dustin->visible = true; dustin->start(); add(r + 3); break;
    case 3: dustin->visible = true; dustin->start(); add(r + 2); break;
    default: break;
    }
}

void PixieDustBag::fill() {                          // 0x4f0770
    dustin->visible = true;
    dustin->start();
    add(50);
}

void PixieDustBag::refund() {                        // 0x4f07b0
    switch (state().skill) {
    case 1: dustin->visible = true; dustin->start(); add(2); break;
    case 2: dustin->visible = true; dustin->start(); add(3); break;
    case 3: dustin->visible = true; dustin->start(); add(4); break;
    default: break;
    }
}

void PixieDustBag::spend() {                         // 0x4f0860
    puff.visible = true;
    puff.setFrame(0);
    add(-by_skill(4, 5, 6, 0));
}

int PixieDustBag::cost2() { return by_skill(4, 5, 6, 0); }   // 0x4f08c0

// 0x4f08f0: two more every 15 / 17 / 19 s (other skills: the caller's value, which is garbage in the exe).
void PixieDustBag::regen(uint32_t unknown) {
    uint32_t now = frame_time();
    uint32_t every = (uint32_t)by_skill(15000, 17000, 19000, (int)unknown);
    if (now - regen_time < every) return;
    add(2);
    regen_time = now;
}

}  // namespace Game
