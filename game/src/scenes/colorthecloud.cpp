// The Color the Clouds game, reconstructed from "Tinker Bell.exe" (see colorthecloud.h).
#include "colorthecloud.h"
#include "continue_dialog.h"
#include "cannon.h"
#include "state.h"
#include "toolbar.h"
#include "raytrace_scene.h"
#include "voiceover.h"
#include "../../../platform/dinput.h"
#include "tb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Vision { void blit_forget(const Pixelmap *pm); }

namespace Game {

using namespace Vision;

static bool key_down(int vk) { return tb_GetKeyState(vk) < 0; }   // 0x4183f0
static void dbg(const char *m) { if (getenv("TB_DEBUG_CLOUD")) tb_log("colorthecloud: %s (t=%u)", m, frame_time()); }

ColorTheCloud_Scene &colorthecloud_scene() {         // the static at 0x5d0058
    static ColorTheCloud_Scene s;
    return s;
}
static ColorTheCloud_Scene &S() { return colorthecloud_scene(); }

// The player's colour (the game state's +0x6c, chosen with the frame in the sign-in room)
static CTCColour &player_colour() {
    static CTCColour c;
    c.set(Game::state().frame_color);
    return c;
}

// ---------------------------------------------------------------- CTCColour
void CTCColour::set(int n) {                         // 0x4ef7b0
    id = n;
    switch (n) {
    case 0: r = 0; g = 0; b = 0xff; break;           // blue
    case 1: r = 0; g = 0xff; b = 0; break;           // green
    case 2: r = 0xff; g = 0xa0; b = 0x58; break;     // orange
    case 3: r = 0xcb; g = 0x66; b = 0xf8; break;     // purple
    case 4: r = 0xff; g = 0; b = 0; break;           // red
    case 5: r = 0xff; g = 0xff; b = 0; break;        // yellow
    }
}

const char *CTCColour::name() const {                // 0x4ef830
    static const char *const n[6] = {"blu", "grn", "org", "pur", "red", "yel"};
    return (unsigned)id < 6 ? n[id] : "";
}

// ---------------------------------------------------------------- CTCPaintBalloon
CTCPaintBalloon::CTCPaintBalloon() : Graphic(GRAPHIC_ARGS) { z = 50; }   // (+0x60 set directly)

// 0x46e560: thrown from x, y in the colour's balloon
void CTCPaintBalloon::throwAt(int px, int py, CTCColour *c) {
    x = px;
    y = py;
    colour = c;
    busy = true;
    t0 = frame_time();
    bursting = false;
    bal_frame = 0;
    burst_frame = 0;
    ColorTheCloud_Scene &s = S();
    int id = c->id;
    if ((unsigned)id <= 5) {
        bal = s.bal_anims[id];
        burst = s.burst_anims[id];
    }
}

// 0x46e6a0: the balloon falls (its frames), then bursts, splashing the cloud under it on each burst frame. Peter's
// on the hard game run two frames a step and splash a few frames early. The time test's start is never moved on,
// so after the first 50 ms it steps on every draw: kept.
void CTCPaintBalloon::draw() {
    if (!busy) return;
    ColorTheCloud_Scene &s = S();
    bool hard_peter = Game::state().skill == 3 && colour == &s.peter_colour;
    if (frame_time() - t0 >= delay) {
        if (bursting) {
            if (burst_frame == 0 || !hard_peter) s.splash(x, y, colour, 3);
            if (burst_frame >= burst->count() + 1) {
                busy = false;
                return;
            }
            burst_frame++;
            burst->setFrame(burst_frame);
        } else {
            if (bal_frame >= bal->count() - 1) {
                bursting = true;
            } else {
                bal_frame++;
                if (hard_peter) bal_frame += 2;
                bal->setFrame(bal_frame);
            }
            if (hard_peter && bal_frame >= bal->count() - 4) s.splash(x, y, colour, 3);
        }
    }
    Animation *a = bursting ? burst : bal;
    a->x = x;
    a->y = y;
    a->draw();
}

// ---------------------------------------------------------------- CTCCursor
CTCCursor::CTCCursor()                               // 0x46d3c0
    : Graphic(GRAPHIC_ARGS),
      active("COLR_CU_active_01", 0, 0, 0, 0, 0, 0, true),
      notactive("COLR_CU_notactive_01", 0, 0, 0, 0, 0, 0, true),
      normal("GLOB_CU_normal_01", 0, 0, 0, 0, 0, 0, true),
      hand_anim("glob_cu_active_"),
      sound("COLR_FX_CUactivate01") {
    hand = false;
    ready = false;
    active.load();
    notactive.load();
    normal.load();
    hand_anim.load();
    hand_anim.play(0, 1);                            // 0x403be0
    chime = 1;
}

CTCCursor::~CTCCursor() {                            // 0x46d560
    active.unload();
    notactive.unload();
    normal.unload();
    sound.close();
}

// v14 0x46d650: over the map icon (or with the fuse out) the hand, or the plain cursor while a drop is not ready;
// else the paint cursor, lit when ready (the chime the first time it lights).
void CTCCursor::draw() {
    if (hand) {
        if (ready) {
            hand_anim.x = x;
            hand_anim.y = y;
            hand_anim.draw();
        } else {
            normal.x = x;
            normal.y = y;
            normal.draw();
        }
        return;
    }
    if (ready) {
        if (chime == 0) {
            sound.play();
            chime = 2;
        }
        active.x = x;
        active.y = y;
        active.draw();
    } else {
        notactive.x = x;
        notactive.y = y;
        notactive.draw();
    }
}

void CTCCursor::setReady(bool v) {                   // 0x46d730
    if (!sound.isOpen()) sound.open();
    ready = v;
    if (v) {
        if (chime == 1) chime = 0;
    } else {
        chime = 1;
    }
}

// ---------------------------------------------------------------- ColorableCloud
ColorableCloud::ColorableCloud(const char *name)     // 0x46d790
    : Graphic(GRAPHIC_ARGS), img(name, 0, 0, 0, 0, 0, 0, true) {
    y = 0;
    x = 0;
    setZ(100);                                       // 0x64
    active = false;
    for (int i = 0; i < 5; i++) splats[i].active = false;
    every = 50;                                      // 0x32
    t = frame_time();
    img.load();
    maxr = img.oh;                                   // +0xd8
    if (img.ow > maxr) maxr = img.ow;                // +0xd4
    if (saved.empty() && img.pixelmap) {
        // the pixels as loaded (the record's size word just before them)
        Pixelmap *pm = img.pixelmap;
        uint32_t n = pm->pixels >= 4 ? pm->u32(pm->pixels - 4) : 0;
        if (pm->pixels + n > pm->data.size()) n = (uint32_t)pm->data.size() - pm->pixels;   // port: bounded
        saved.assign(pm->data.begin() + pm->pixels, pm->data.begin() + pm->pixels + n);
    }
    colour = 6;
}

ColorableCloud::~ColorableCloud() {                  // 0x46d930
    img.unload();
}

Pixelmap *ColorableCloud::pixelmap() { return img.pixelmap; }

// v14 0x46d9d0: every 50 ms each splat's ring grows by its step (the first one a disc of 30), painted on; past the
// cloud's size it stops. Then the cloud.
void ColorableCloud::draw() {
    uint32_t now = frame_time();
    if (now - t >= every) {
        t = now;
        for (int i = 0; i < 5; i++) {
            Splat &s = splats[i];
            if (!s.active) continue;
            if (s.r0 == 0 && s.r1 == 0) {
                s.r1 = 30;                           // 0x1e
                paint(s.x, s.y, s.r0, s.r1, s.colour->r, s.colour->g, s.colour->b);
                s.r0 = s.r1 - s.step;
            } else {
                s.r0 += s.step;
                s.r1 += s.step;
                if (s.r0 > maxr) s.active = false;
                else paint(s.x, s.y, s.r0, s.r1, s.colour->r, s.colour->g, s.colour->b);
            }
        }
    }
    img.x = x;
    img.y = y;
    img.draw();
}

// 0x46dae0: the ring r0 <= d < r1 around cx, cy blended a quarter towards the colour, from the white pixels; for the
// three pixel layouts (raw, and the two run-length ones).
void ColorableCloud::paint(int cx, int cy, int r0, int r1, uint8_t R, uint8_t G, uint8_t B) {
    pr = R;
    pg = G;
    pb = B;
    uint32_t rgb = (uint32_t)R << 16 | (uint32_t)G << 8 | B;
    // the display's masks and shifts (0x5a67e8/ec/f4, 0x5a65c4/0x5a67d0/0x5a65c8): RGB565 here
    const uint32_t BM = 0x1f, GM = 0x7e0, RM = 0xf800;
    uint32_t col = ((rgb >> 3) & BM) | ((rgb >> 5) & GM) | ((rgb >> 8) & RM);
    Pixelmap *pm = img.pixelmap;
    if (!pm || saved.empty()) return;
    uint8_t *dst = pm->data.data() + pm->pixels, *dend = pm->data.data() + pm->data.size();
    const uint8_t *src = saved.data(), *send = saved.data() + saved.size();
    int w = pm->w, h = pm->h;
    int row = y - h / 2;
    int rr0 = r0 * r0, rr1 = r1 * r1;
    auto blend = [&](uint8_t *d, const uint8_t *s) {
        if (d + 2 > dend || s + 2 > send) return;
        uint32_t o = (uint32_t)(s[0] | s[1] << 8);
        uint32_t v = ((((o & BM) * 3 + (col & BM)) << 6 >> 8) & BM) | ((((o & GM) * 3 + (col & GM)) << 6 >> 8) & GM) |
                     ((((o & RM) * 3 + (col & RM)) << 6 >> 8) & RM);
        d[0] = (uint8_t)v;
        d[1] = (uint8_t)(v >> 8);
    };
    int fmt = pm->kind & ~0x10;
    if (h <= 0) return;
    if (fmt == 0) {
        for (int r = 0; r < h; r++, row++) {
            int px = x - w / 2;
            int dy2 = (row - cy) * (row - cy);
            if (dy2 >= rr1) {
                dst += w * 2;
                src += w * 2;
                continue;
            }
            for (int c = 0; c < w; c++, px++, dst += 2, src += 2) {
                int d2 = (px - cx) * (px - cx) + dy2;
                if (d2 < rr1 && d2 >= rr0) blend(dst, src);
            }
        }
    } else if (fmt == 1 || fmt == 2) {
        for (int r = 0; r < h; r++, row++) {
            int px = x - w / 2;
            int dy2 = (row - cy) * (row - cy);
            int n = 0;
            while (n < w && dst < dend) {
                uint32_t k = *dst++;
                src++;
                if (fmt == 2 && k == 0xff) {
                    k = (uint32_t)(dst[0] | dst[1] << 8);
                    dst += 2;
                    src += 2;
                }
                n += (int)k;
                px += (int)k;
                if (n >= w) break;
                uint32_t m = *dst++;
                src++;
                if (fmt == 2 && m == 0xff) {
                    m = (uint32_t)(dst[0] | dst[1] << 8);
                    dst += 2;
                    src += 2;
                }
                for (uint32_t i = 0; i < m; i++, px++, dst += 2, src += 2) {
                    int d2 = (px - cx) * (px - cx) + dy2;
                    if (d2 < rr1 && d2 >= rr0) blend(dst, src);
                }
                n += (int)m;
            }
        }
    }
    blit_forget(pm);                                 // port: the blitter keeps a decoded copy of each image
}

// 0x46e080: a new splat (if one of the five is free); the cloud now counts for that colour
void ColorableCloud::addSplat(int px, int py, CTCColour *c, int step) {
    int i = 0;
    for (; i < 5; i++)
        if (!splats[i].active) break;
    if (i >= 5) return;
    colour = c->id;
    Splat &s = splats[i];
    s.colour = c;
    s.x = px;
    s.y = py;
    s.step = step;
    s.active = true;
    s.r0 = 0;
    s.r1 = 0;
}

// ---------------------------------------------------------------- CTCInterface
static Image *new_image(const char *n) {
    Image *i = Gorilla::gnew<Image>(n, 0, 0, 0, 0, 0, 0, true);
    i->load();
    return i;
}

CTCInterface::CTCInterface() : Graphic(GRAPHIC_ARGS), bar("COLR_IF_interface_01", 0, 0, 0, 0, 0, 0, true) {   // 0x46ef70
    char buf[64];
    bar.load();
    snprintf(buf, sizeof buf, "COLR_IF_smcld%s_01", player_colour().name());
    mine = new_image(buf);
    mine->x = -288;                                  // 0xfffffee0
    mine->y = -27;                                   // 0xffffffe5
    snprintf(buf, sizeof buf, "COLR_IF_smcld%s_01", S().peter_colour.name());
    his = new_image(buf);
    his->x = 288;                                    // 0x120
    his->y = -27;
    for (int i = 0; i < 10; i++) {
        snprintf(buf, sizeof buf, "GLOB_IF_NUM%dsm_01", i);
        digits.push_back(new_image(buf));
    }
    for (int i = 1; i <= 4; i++) {
        snprintf(buf, sizeof buf, "COLR_IF_target_%2.2d", i);
        targets.push_back(new_image(buf));
    }
}

CTCInterface::~CTCInterface() {                      // 0x46f440
    bar.unload();
    for (Image *i : {mine, his}) {
        if (!i) continue;
        i->unload();
        delete i;
    }
    for (Image *i : digits) { i->unload(); delete i; }
    for (Image *i : targets) { i->unload(); delete i; }
}

// v14 0x46f680: the bar, the two small clouds, and the scores' digits at the target images' own places (no tens
// digit under ten)
void CTCInterface::draw() {
    bar.draw();
    mine->draw();
    his->draw();
    auto put = [&](int d, int t) {
        Image *g = digits.at(d);
        g->x = targets.at(t)->ox;                    // +0x30
        g->y = targets.at(t)->oy;                    // +0x34
        g->draw();
    };
    if (p / 10) put(p / 10, 0);
    put(p % 10, 1);
    if (s / 10) put(s / 10, 2);
    put(s % 10, 3);
}

// ---------------------------------------------------------------- CloudSpawner
CloudSpawner::~CloudSpawner() {}                     // 0x46e100

// 0x46e150: at most once a second, the first cloud not on its way (one in ten tries each) starts at the top
void CloudSpawner::spawn() {
    uint32_t now = frame_time();
    if (now - t < 1000) return;                      // 0x3e8
    for (int i = 0; i < 10; i++) {
        ColorableCloud *c = scene->clouds_[i];
        if (c->active) continue;
        if (vision_random(1000) >= 100) continue;
        t = now;
        c->active = true;
        c->x = vision_random(600) - 300;             // 0x258, 0x12c
        c->y = -350;                                 // 0xfffffea2
        return;
    }
}

// 0x46e1f0: each cloud on its way moves with the ground (and its splats with it) and down at its speed; past the
// bottom it scores its points for its last colour (the player's or Peter's), then is washed white again.
void CloudSpawner::update() {
    ColorTheCloud_Scene &s = *scene;
    for (int i = 0; i < 10; i++) {
        ColorableCloud *c = s.clouds_[i];
        if (!c->active) continue;
        auto shift = [&](int dx, int dy) {
            c->x += dx;
            c->y += dy;
            for (int k = 0; k < 5; k++)
                if (c->splats[k].active) {
                    c->splats[k].x += dx;
                    c->splats[k].y += dy;
                }
        };
        if (s.left && s.ground->x < 64) shift(4, 0);
        if (s.right && s.ground->x > -64) shift(-4, 0);
        if (s.up) shift(0, 2);
        if (s.down) shift(0, -2);
        shift(0, c->speed);
        if (c->y <= 350) continue;                   // 0x15e
        int col = c->colour;
        if (col == player_colour().id) {
            s.iface->p += c->kind;
            if (s.iface->p > 99) s.iface->p = 99;
            s.sounds[3].play();                      // 0x5d0c08 plascore
            if (vision_random(5) != 0 && !Game::state().vo_busy()) {   // state +8, +0x560
                int k = vision_random((int)nice.size());
                VoiceOver::get().play(nice.at(k));
            }
        }
        if (col == s.peter_colour.id) {
            s.iface->s += c->kind;
            if (s.iface->s > 99) s.iface->s = 99;
            if (!s.sounds[4].isPlaying()) s.sounds[4].play();   // 0x5d0f50 oppscore
        }
        c->colour = 6;
        for (int k = 0; k < 5; k++) c->splats[k].active = false;
        Pixelmap *pm = c->pixelmap();
        if (pm && !c->saved.empty()) {
            std::copy(c->saved.begin(), c->saved.end(), pm->data.begin() + pm->pixels);
            blit_forget(pm);                         // port: see paint
        }
        c->active = false;
    }
}

// ---------------------------------------------------------------- PeterAI
void PeterAI::setState(int s) {                      // 0x46e880
    counter = 0;
    state = s;
    if (s == 0 || s == 1) dir = 666;
    else if (s == 3) dir = 667;
}

// 0x46e8b0: the nearest cloud on its way that a balloon dropped now would still meet on the screen: its x, and a y
// ahead of it by its speed (and up to 70 more at random)
bool PeterAI::target() {
    ColorTheCloud_Scene &s = S();
    int best = 100000000;                            // 0x5f5e100
    bool found = false;
    for (int i = 0; i < 10; i++) {
        ColorableCloud *c = s.clouds_[i];
        if (!c->active) continue;
        int dx = c->x - r->x, dy = c->y - r->y;
        int d2 = dy * dy + dx * dx;
        if (d2 > best) continue;
        int ny = vision_random(70) + abs(dx) + c->speed * 20 + c->y + 110;   // 0x46, 0x6e
        if (ny > 200 || ny < -200) continue;
        ty = ny;
        tx = c->x;
        best = d2;
        found = true;
    }
    return found;
}

// 0x46e970: as Bird Spy's Peter: on along a way, or through the take-off frames into a new one; the throw too.
void PeterAI::animate() {
    Animation *str = nullptr, *fly = nullptr;
    switch (dir) {
    case 0: str = r->find("start_north"); fly = r->find("fly_north"); break;
    case 2: str = r->find("start_east"); fly = r->find("fly_east"); break;
    case 4: str = r->find("start_south"); fly = r->find("fly_south"); break;
    case 6: str = r->find("start_west"); fly = r->find("fly_west"); break;
    case 666: fly = str = r->find("fly_idle"); break;
    case 667: fly = str = r->find("bomb"); break;
    }
    if (cur == fly) {
        cur->step();
        lastdir = dir;
        return;
    }
    if (cur == str) {
        cur->step();
        if (cur->current() < cur->count() - 1) {
            r->select(cur, true);
            return;
        }
        cur = fly;
        fly->setFrame(0);
        lastdir = dir;
        r->select(cur, true);
        return;
    }
    if (cur == r->find("fly_idle")) {
        cur = str;
        str->setFrame(0);
        r->select(cur, true);
        return;
    }
    if (cur == r->find("bomb")) {
        if (cur->current() < cur->count() - 1) return;
        cur = str;
        str->setFrame(0);
        r->select(cur, true);
        return;
    }
    const char *back = nullptr;
    if (cur == r->find("fly_north")) back = "start_north";
    else if (cur == r->find("fly_south")) back = "start_south";
    else if (cur == r->find("fly_east")) back = "start_east";
    else if (cur == r->find("fly_west")) back = "start_west";
    if (back) {
        cur = r->find(back);
        cur->setFrame(cur->count() - 1);
    } else if (cur->current() != 0) {
        cur->setFrame(cur->current() - 1);
    } else if (dir == 666) {
        cur = r->find("fly_idle");
        cur->setFrame(0);
    } else {
        cur = str;
        str->setFrame(0);
    }
    r->select(cur, true);
}

// 0x46ebf0: mostly (by the skill) off to a cloud he can hit, or anywhere near; else, after two seconds (at once on
// the hard game), a throw when a cloud is below him (or one time in ten anyway). The test for a "throw" animation
// never holds (there is none; his is "bomb"): kept.
void PeterAI::decide() {
    ColorTheCloud_Scene &s = S();
    if (r->isCur("throw")) return;
    int n = vision_random(1000);
    if ((double)s.peter_move * (double)0.01f * (double)222.0f > (double)n) {
        if (!target()) {
            tx = vision_random(200) + r->x - 100;
            if (tx < -250) tx += 100;
            if (tx > 250) tx -= 100;
            ty = vision_random(200) + r->y - 100;
            if (ty < -100) ty += 100;
            if (ty > 200) ty -= 100;
        }
        int dx = tx - r->x, dy = ty - r->y;
        if (abs(dx) > abs(dy)) dir = dx > 0 ? 2 : 6;
        else dir = dy > 0 ? 4 : 0;
        if (stop) return;
        state = 2;
        counter = 0;
        return;
    }
    if (!(frame_time() - t > 2000 || Game::state().skill == 3)) return;
    if (stop) return;
    for (int i = 0; i < 10; i++) {
        ColorableCloud *c = s.clouds_[i];
        if (!c->active) continue;
        int a = vision_random(100);
        int e = vision_random(100) + a;
        for (int k = a; k < e; k += 10) {
            c->img.x = c->x;
            c->img.y = c->y;
            if (c->img.hit(r->x, r->y - k)) {
                setState(3);
                return;
            }
        }
    }
    if ((unsigned)vision_random(1000) < 100) setState(3);
}

// 0x46ee10: a sixth of the way there (four more, the exe's sum), until within 6
void PeterAI::move() {
    int dx = tx - r->x, dy = ty - r->y;
    auto stepof = [](int d) {
        int st = d / 6;
        if (d > st) st += 4;
        else if (d < st) {
            st -= 4;
            if (d > st) st += 4;
        }
        return st;
    };
    r->x += stepof(dx);
    r->y += stepof(dy);
    if (abs(dx) < 6 && abs(dy) < 6) {
        state = 1;
        counter = 0;
        dir = 666;
    }
}

// 0x46eeb0: the throw: on its third frame a balloon of his colour from just above him
void PeterAI::throwing() {
    ColorTheCloud_Scene &s = S();
    if (counter == 0) {
        counter = 1;
        t = frame_time();
        return;
    }
    if (counter == 1) {
        if (r->frame() == 2) {
            s.sounds[1].play();                      // 0x5d0578 oppdrop
            for (int i = 0; i < 10; i++) {
                if (s.balloons[i]->busy) continue;
                s.balloons[i]->throwAt(r->x, r->y - 100, &s.peter_colour);
                break;
            }
            counter = 2;
        }
    }
    if (r->atEnd()) {
        state = 1;
        counter = 0;
        dir = 666;
    }
}

// ---------------------------------------------------------------- ColorTheCloud_Scene
ColorTheCloud_Scene::ColorTheCloud_Scene()           // 0x46ce80
    : Game2D_Scene("colorthecloud"),
      guide("COLR_VO_PETgreet1", "COLR_VO_PETgreet2"),
      music(nullptr),
      clicky("COLR_FX_CUclicky"),
      clickn("COLR_FX_CUclickn") {
    keyboard.setClient(this);
    mouse.setClient(this);
}

ColorTheCloud_Scene::~ColorTheCloud_Scene() {}       // 0x46d080

// v18 0x46bf80
void ColorTheCloud_Scene::load2() {
    music.setName("COLR_MU_music");
    music.mode = 2;                                  // +0x2050
    music.type = 6;                                  // +0x2058
    static const char *const names[5] = {"colr_fx_pladrop", "colr_fx_oppdrop", "colr_fx_burst", "colr_fx_plascore",
                                         "colr_fx_oppscore"};   // 0x53b6c4
    for (int i = 0; i < 5; i++) sounds[i].setName(names[i]);
    inactivity.add("COLR_VO_PETprompt", false);
    endvo.setWins("COLR_VO_PETtbwins_01", "COLR_VO_PETtbwins_02", "COLR_VO_PETtbwins_03");
    endvo.setLoses("COLR_VO_PETtblose_01", "COLR_VO_PETtblose_02", "COLR_VO_PETtblose_03");
    endvo.setTie("COLR_VO_PETtie");
    endvo.setWinChallenge("COLR_VO_PETwinsch_01", "COLR_VO_PETwinsch_02", "COLR_VO_PETwinsch_03");
    endvo.setAward("COLR_VO_PETaward");
    endvo.setLoseChallenge("COLR_VO_PETlosech_01", "COLR_VO_PETlosech_02", "COLR_VO_PETlosech_03");
    endvo.setTie2("COLR_VO_PETtie");
    stops.add("COLR_VO_PETstop1");                   // 0x4cc0d0
    stops.add("COLR_VO_PETstop2");
    stops.add("COLR_VO_PETstop3");
}

// v19 0x46c900: input; the "color_the_cloud" pack; the cursor (hidden); Peter's guide (unless coming back); the end
// scoreboard; Peter's colour (any but the player's); the score bar; the ground; the cannon; the skill's numbers;
// Peter; the fuse shown at 60 seconds; the sounds.
void ColorTheCloud_Scene::enter() {
    Game2D_Scene::enter();
    mouse.activate();
    keyboard.activate();
    pack = ras_open("color_the_cloud", nullptr, &data_device());
    pack->load();
    ras_set_current(pack);
    escaped = false;
    over = false;
    playing = false;
    cursor = Gorilla::gnew<CTCCursor>();             // 0x4243c0(0x6f8)
    cursor->visible = false;
    set_cursor(cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
    hide_cursor();
    screen.activate();
    screen.loadAll();
    if (previous != this) {                          // +0xc4
        guide.load(false);                           // 0x4ceb60
        guide.setZ(-975);                            // 0xfffffc31
        screen.layer(0)->add(&guide);
        guide.Task::activate();                      // +0x12d8 v6
    }
    reset();                                         // 0x421110
    activate();
    last = 0;
    board = Gorilla::gnew<Scoreboard>("COLR_IF_scorescr_01");
    board->setZ(-1000);
    board->visible = false;
    screen.layer(0)->add(board);
    int r;
    do r = vision_random(6);                         // the scene's random +0x1c4
    while (r == player_colour().id);
    peter_colour.set(r);
    iface = Gorilla::gnew<CTCInterface>();           // 0x4243c0(0x170)
    iface->setZ(-1000);
    screen.layer(0)->add(iface);
    ground = Gorilla::gnew<ScrollGround>("COLR_BG_ground1_01", "COLR_BG_ground2_01");
    ground->load();
    screen.layer(0)->add(ground);
    cannon = &Game::cannon();                        // state +0x7c0
    cannon->visible = true;
    screen.layer(0)->add(cannon);
    switch (Game::state().skill) {                   // +0x68
    case 1: peter_move = 60.0f; nsmall = 2; nmedium = 5; speed_pct = 70.0f; break;
    case 2: peter_move = 70.0f; nsmall = 3; nmedium = 5; speed_pct = 80.0f; break;
    case 3: peter_move = 90.0f; nsmall = 5; nmedium = 4; speed_pct = 100.0f; break;
    }
    setupPeter();                                    // 0x46c750
    cannon->setTime(60);                             // 0x3c
    music.open();
    for (int i = 0; i < 5; i++) sounds[i].open();
    clicky.open();
    clickn.open();
}

// 0x46c0a0: ten clouds, small ones first (by the skill's counts), then medium, the rest large; their speeds by size;
// above the screen, on top of each other by kind. Then the spawner with Peter's "nice" lines.
void ColorTheCloud_Scene::setupClouds() {
    int ns = 0, nm = 0, nl = 0;
    char buf[64];
    for (int i = 0; i < 10; i++) {
        int kind, speed, zz;
        if (ns < nsmall) {
            ns++;
            snprintf(buf, sizeof buf, "colr_ga_cloudsm%1.1d_01", ns);
            speed = ftol((double)speed_pct * (double)0.01f * (double)12.0f);
            zz = 0xdc + i - 0x14;
            kind = 3;
        } else if (nm < nmedium) {
            nm++;
            snprintf(buf, sizeof buf, "colr_ga_cloudmd%1.1d_01", nm);
            speed = ftol((double)speed_pct * (double)0.01f * (double)10.0f);
            zz = 0xdc + i;
            kind = 2;
        } else {
            nl++;
            snprintf(buf, sizeof buf, "colr_ga_cloudlg%1.1d_01", nl);
            speed = ftol((double)speed_pct * (double)0.01f * (double)8.0f);
            zz = 0xdc + i + 0xa;
            kind = 1;
        }
        ColorableCloud *c = Gorilla::gnew<ColorableCloud>(buf);   // 0x4243c0(0x210)
        clouds_[i] = c;
        c->kind = kind;
        c->load();
        c->y = -300;                                 // 0xfffffed4
        c->speed = speed;
        c->setZ(zz);
        screen.layer(0)->add(c);
    }
    spawner = new CloudSpawner();                    // 0x4f34f3(0x1c)
    spawner->scene = this;
    spawner->t = frame_time();
    spawner->nice.push_back("COLR_VO_PETnice1");
    spawner->nice.push_back("COLR_VO_PETnice2");
    spawner->nice.push_back("COLR_VO_PETnice3");
    spawner->nice.push_back("COLR_VO_PETnice4");     // (an insert at the end)
}

// 0x46c470: each colour's balloon and burst; ten balloons
void ColorTheCloud_Scene::setupBalloons() {
    static const char *const cn[6] = {"blu", "grn", "org", "pur", "red", "yel"};   // 0x46c6b0
    char buf[64];
    for (int i = 0; i < 6; i++) {
        snprintf(buf, sizeof buf, "colr_ga_bal%s_", cn[i]);
        bal_anims[i] = Gorilla::gnew<Animation>(buf);
        bal_anims[i]->load();
        snprintf(buf, sizeof buf, "colr_ga_burst%s_", cn[i]);
        burst_anims[i] = Gorilla::gnew<Animation>(buf);
        burst_anims[i]->load();
    }
    for (int i = 0; i < 10; i++) {
        balloons[i] = Gorilla::gnew<CTCPaintBalloon>();
        screen.layer(0)->add(balloons[i]);
    }
}

void ColorTheCloud_Scene::freeBalloons() {           // 0x46c6d0
    for (int i = 0; i < 6; i++) {
        if (bal_anims[i]) bal_anims[i]->unload();
        delete bal_anims[i];
        bal_anims[i] = nullptr;
        if (burst_anims[i]) burst_anims[i]->unload();
        delete burst_anims[i];
        burst_anims[i] = nullptr;
    }
    for (int i = 0; i < 10; i++) {
        delete balloons[i];
        balloons[i] = nullptr;
    }
}

// 0x46c750: Peter's flight, take-offs and throw; his thinking
void ColorTheCloud_Scene::setupPeter() {
    peter_r = Gorilla::gnew<TopDownCharacterRenderer>();   // 0x4243c0(0x100)
    peter_r->add("fly_idle", "colr_ca_petflyidle_");
    peter_r->add("fly_north", "colr_ca_petflyno_");
    peter_r->add("fly_south", "colr_ca_petflyso_");
    peter_r->add("fly_east", "colr_ca_petflyea_");
    peter_r->add("fly_west", "colr_ca_petflywe_");
    peter_r->add("start_north", "colr_ca_petstrno_");
    peter_r->add("start_south", "colr_ca_petstrso_");
    peter_r->add("start_east", "colr_ca_petstrea_");
    peter_r->add("start_west", "colr_ca_petstrwe_");
    peter_r->add("bomb", "colr_ca_petthrow_");
    peter_r->load();
    screen.layer(0)->add(peter_r);
    ai = new PeterAI();                              // 0x4f34f3(0x2c)
    ai->state = 0;
    ai->t = frame_time();
    ai->stop = false;
    ai->r = peter_r;
}

// 0x46d310: the front cloud under x, y (on its way) gets a splat, with the burst's sound
void ColorTheCloud_Scene::splash(int x, int y, CTCColour *c, int step) {
    ColorableCloud *best = nullptr;
    int bz = 1000;                                   // 0x3e8
    for (int i = 0; i < 10; i++) {
        ColorableCloud *cl = clouds_[i];
        if (!cl->active) continue;
        cl->img.x = cl->x;
        cl->img.y = cl->y;
        if (!cl->img.hit(x, y)) continue;
        if (cl->z < bz) {
            bz = cl->z;
            best = cl;
        }
    }
    if (!best) return;
    if (!sounds[2].isPlaying()) sounds[2].play();    // +0x868 burst
    best->addSplat(x, y, c, 3);
    (void)step;
}

// The cloud loops keep Peter hovering
void ColorTheCloud_Scene::peterIdleStep() {
    if (!peter_r->isCur("fly_idle")) peter_r->select("fly_idle", true);
    ai->cur = peter_r->find("fly_idle");
    ai->cur->step();
}

// The cursor each step: the hand over the map icon or once the fuse is out; ready a second after the last drop
void ColorTheCloud_Scene::cursorUpdate() {
    bool hit = map->hit(g_mouse.x, g_mouse.y);       // v11
    bool nofuse = !cannon->timeLeft();
    bool elapsed = frame_time() - last > 1000;       // 0x3e8
    cursor->hand = hit || nofuse;
    cursor->setReady((elapsed && !nofuse) ? true : hit);
}

// v10 0x46b260
void ColorTheCloud_Scene::run() {
    State &st = Game::state();
    wait(1);
    hide_cursor();
    clouds->reset();
    dbg("run");
    while (guide.Task::active()) wait(0);            // +0x12d8 v5
    setupClouds();                                   // 0x46c0a0
    setupBalloons();                                 // 0x46c470
    ground->visible = true;
    music.play();
    cursor->setReady(true);
    cursor->visible = true;
    show_cursor();
    while (clouds->fall()) {
        peterIdleStep();
        wait(30);
    }
    inactivity.start();                              // v12
    VoiceOver::get().play("COLR_VO_PETchabeg_01");   // 0x4c9400
    while (st.vo_busy()) wait(30);                   // +0x560
    cannon->restart();                               // 0x4e3b30
    playing = true;
    dbg("play");
    while (!escaped && cannon->timeLeft() && iface->p < 99 && iface->s < 99) {
        wait(30);
        if (ai->state) ai->animate();
        switch (ai->state) {
        case 0:
            ai->dir = 666;
            ai->lastdir = 666;
            ai->cur = peter_r->find("fly_idle");
            ai->setState(1);
            break;
        case 1: ai->decide(); break;
        case 2: ai->move(); break;
        case 3: ai->throwing(); break;
        }
        if (spawner->scene->over) {
            for (int i = 0; i < 10; i++) clouds_[i]->active = false;
        } else {
            spawner->spawn();
            spawner->update();
        }
        cursorUpdate();
    }
    if (getenv("TB_DEBUG_CLOUD"))
        tb_log("colorthecloud: over: escaped %d fuse %d p %d s %d", escaped, cannon->timeLeft(), iface->p, iface->s);
    ai->stop = true;
    ai->setState(1);
    hide_cursor();
    if (escaped) {
        fader->visible = true;
        fader->start(false, 2000);
        hide_cursor();
        while (fader->busy()) wait(0);
        change_scene("Starry");
    }
    cursor->setReady(false);
    cursor->visible = false;
    over = true;
    inactivity.stop();                               // v13
    clouds->cover();
    while (clouds->rise()) {
        peterIdleStep();
        wait(30);
    }
    board->s2 = iface->s;
    board->s1 = iface->p;
    board->visible = true;
    wait(3000);
    if (iface->p > iface->s) {
        int info = st.saves.gameInfo("Color The Cloud Game");   // *0x543500
        if (info == 0) st.last_result = 2;
        else if (info == 1) st.last_result = 3;
        else if (info == 2) st.last_result = 4;
    } else if (iface->p == iface->s) {
        st.last_result = 5;
    } else if (iface->p < iface->s) {
        st.last_result = 1;
    }
    endvo.playEnd();
    while (endvo.playing()) wait(30);
    if (iface->p > iface->s) {
        change_scene("Starry");
    } else {
        dialog = Gorilla::gnew<ContinueDialog>("glob_if_contgame_01", "glob_if_signglow_01", "glob_if_xglow_01");
        dialog->setAsk("colr_vo_petbegcon_01");
        dialog->setYes("colr_vo_petbegyes_01");
        dialog->setNo("colr_vo_petbegno_01");
        dialog->setIdle("colr_vo_petbegidl_01");
        keyboard.deactivate();                       // v7
        show_cursor();
        call(dialog);
        wait(1);
    }
    for (;;) {                                       // (never finishes: the cursor kept up to date)
        cursorUpdate();
        wait(30);
    }
}

// v9 0x46b1b0: the dialog's answer: again or on to the starry skies
void ColorTheCloud_Scene::notify2(Service *s) {
    if (current_scene() != this) return;
    if (s == &mouse) {
        onMouse();
        return;
    }
    if (s == &keyboard) {
        onKey();
        return;
    }
    if (s == dialog) {
        if (dialog->yes) {                           // +0x6f1
            if (board) board->visible = false;
            again();                                 // 0x41dde0
        } else if (dialog->no) {                     // +0x6f0
            change_scene("Starry");
        }
        dialog = nullptr;
        mouse.activate();
        keyboard.activate();
    }
}

// 0x46ae90: the map icon; else, a second after the last, a balloon of the player's colour where he clicked (and
// Peter, clicked, says to stop it); too soon, the "no" click.
void ColorTheCloud_Scene::onMouse() {
    if (!g_cursor_graphic || !g_cursor_graphic->visible) return;
    if (!cursor->visible) return;
    if (!(g_mouse.ldown || g_mouse.rdown)) return;   // 0x5a6e30
    if (!playing) return;
    int r = map->click(g_mouse.x, g_mouse.y, true);
    if (r) {
        if (r == 2) change_scene("map");
        return;
    }
    if (over) return;
    if (frame_time() - last > 1000) {
        clicky.play();
        int mx = g_mouse.x, my = g_mouse.y;
        for (int i = 0; i < 10; i++) {
            if (balloons[i]->busy) continue;
            balloons[i]->throwAt(mx, my, &player_colour());   // state +0x6c
            break;
        }
        last = frame_time();
        sounds[0].play();                            // pladrop
        if (peter_r->hitAt(g_mouse.x, g_mouse.y) && !stops.anyPlaying()) stops.playNext();
        return;
    }
    if (frame_time() - last < 1000) clickn.play();
}

// 0x46b000: space skips Peter's welcome; Escape the game; the arrows scroll and slide the ground
void ColorTheCloud_Scene::onKey() {
    State &st = Game::state();
    if (!playing) {
        if (g_key.vk == 0x20) guide.stop();
        return;
    }
    ground->setMult(2);
    up = down = left = right = false;
    if (g_key.down) {                                // 0x5a6dd8
        if (st.key((int)g_key.vk)) return;
        if ((int)g_key.vk == st.skip_key) {
            escaped = true;
            st.last_game = 5;
        }
    }
    if (over) return;
    if (key_down(0x26) && !over) {
        ground->setMult(4);
        up = true;
    }
    if (key_down(0x28) && !over) {
        ground->setMult(1);
        down = true;
    }
    if (key_down(0x27) && !over) {
        ground->setDir(2);
        right = true;
    }
    if (key_down(0x25) && !over) {
        ground->setDir(1);
        left = true;
    }
    if (left && right) left = right = false;
    if (up && down) {
        up = down = false;
        ground->setMult(2);
    }
    if (!left && !right) ground->setDir(0);
}

// v20 0x46ccb0
void ColorTheCloud_Scene::leave() {
    VoiceOver::get().stop();
    Game2D_Scene::leave();
    guide.unload();                                  // v16
    delete board;                                    // (the exe keeps the pointer)
    board = nullptr;
    music.close();
    for (int i = 0; i < 5; i++) sounds[i].close();
    if (cannon) cannon->detach();
    delete spawner;
    spawner = nullptr;
    for (int i = 0; i < 10; i++) {
        if (clouds_[i]) clouds_[i]->unload();
        delete clouds_[i];
        clouds_[i] = nullptr;
    }
    freeBalloons();                                  // 0x46c6d0
    deactivate();                                    // v7
    if (ground) ground->unload();
    delete ground;
    ground = nullptr;
    delete ai;
    ai = nullptr;
    delete peter_r;
    peter_r = nullptr;
    delete iface;
    iface = nullptr;
    // the cursor goes back to the game's before it is deleted (port: the exe deletes it first and sets the cursor
    // after, leaving the pointer stale for the frames between)
    set_cursor(Game::state().cursor, true);
    delete cursor;
    cursor = nullptr;
    mouse.deactivate();
    keyboard.deactivate();
    screen.deactivate();
    screen.unloadAll();
    ras_release(pack);
    pack = nullptr;
    clicky.close();
    clickn.close();
    tasks_finish_all();                              // 0x4210d0
    set_cursor(Game::state().cursor, true);
    Game::state().guide_loaded = false;
}

}  // namespace Game
