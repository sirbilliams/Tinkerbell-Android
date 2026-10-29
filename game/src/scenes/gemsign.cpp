// GemSign, reconstructed from "Tinker Bell.exe" (see gemsign.h).
#include "gemsign.h"
#include "../vision/engine.h"

namespace Game {

using namespace Vision;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30

// ---------------------------------------------------------------- GemSign
GemSign::GemSign(const char *s, const char *t, const char *g, const char *c, const char *st)   // 0x4e6400
    : names{s, t, g, c, st} {}

void GemSign::init(Raytrace::RayTrace *r) {          // 0x4e6440
    rt = r;
    sprite = rt->findSprite(names[0]);
    if (!sprite) return;                             // port: the exe would crash without the sign
    sprite->attach(names[1], names[1], 100);         // the tick (kept by the sprite only)
    green = sprite->attach(names[2], names[2], 100);
    cap = sprite->attach(names[3], names[3], 90);
    star = sprite->attach(names[4], names[4], 95);
    if (green) green->on = false;
    if (cap) cap->on = false;
    if (star) star->on = false;
    click = new Sound("GLOB_FX_GSclick");            // 0x420210
    click->type = 7;                                 // +0x340
    click->open();
}

void GemSign::release() {                            // 0x4e6530
    sprite = nullptr;
    green = cap = star = nullptr;
    rt = nullptr;
    if (click) { click->close(); delete click; }     // 0x41fc50
    click = nullptr;
}

void GemSign::show() {                               // 0x4e6570
    if (!sprite) return;
    if (sprite->anim) sprite->anim->visible = true;
    rt->makeBox(sprite, false);                      // 0x429580
}

void GemSign::hide() {                               // 0x4e6590
    if (!sprite) return;
    rt->removeBox(sprite);
    if (sprite->anim) sprite->anim->visible = false;
}

bool GemSign::mouse() {                              // 0x4e65b0
    if (!green) return false;
    green->on = false;
    clicked = false;
    if (!enabled) return green->on;
    if (rt->pick(g_mouse.x, g_mouse.y) != sprite) return green->on;
    green->on = true;
    if (!mouse_down()) return green->on;
    clicked = true;
    if (click) click->play();
    return green->on;
}

}  // namespace Game
