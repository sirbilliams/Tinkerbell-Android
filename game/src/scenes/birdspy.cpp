// The Bird Spy game, reconstructed from "Tinker Bell.exe" (see birdspy.h).
#include "birdspy.h"
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

namespace Game {

using namespace Vision;

static bool key_down(int vk) { return tb_GetKeyState(vk) < 0; }   // 0x4183f0
static void dbg(const char *m) { if (getenv("TB_DEBUG_BIRDSPY")) tb_log("birdspy: %s (t=%u)", m, frame_time()); }

BirdSpy_Scene &birdspy_scene() {                     // the static at 0x5c3d08
    static BirdSpy_Scene s;
    return s;
}
static BirdSpy_Scene &S() { return birdspy_scene(); }

// 0x4ec320: the frame's time in seconds
static float now_secs() { return (float)((double)(uint64_t)frame_time() * (double)0.001f); }

// The ten birds by birdIndex, their three letters (in the icons' names)
static const char *const k_code[10] = {"CRA", "CRO", "FLA", "HAW", "HUM", "OWL", "PAR", "PEL", "SPA", "TOU"};
static std::string small_name(int i) { return std::string("BIRD_IF_") + k_code[i] + "small_01"; }     // 0x539770
static std::string smalldk_name(int i) { return std::string("BIRD_IF_") + k_code[i] + "smalldk_01"; } // 0x539798
static const char *const k_dir[8] = {"no", "ne", "ea", "se", "so", "sw", "we", "nw"};   // 0x45c7b8

// The ground scrolls sideways under the arrow keys while it is not at its stop (+-64): a bird or cloud keeps its
// place over the ground by this much a step.
static int ground_dx(int k) {
    BirdSpy_Scene &s = S();
    int v = 0;
    if (s.left && s.ground->x < 64) v = k;           // 0x5c7c72, 0x5c50d4
    if (s.right && s.ground->x > -64) v = -k;        // 0x5c7c73
    return v;
}

// ---------------------------------------------------------------- BirdSpySingleAnim
BirdSpySingleAnim::BirdSpySingleAnim(const char *n, int d) : Graphic(GRAPHIC_ARGS), dir(d) {   // 0x45bef0
    strncpy(name, n, sizeof name - 1);
    name[sizeof name - 1] = 0;
    setZ(50);                                        // 0x32
}
BirdSpySingleAnim::~BirdSpySingleAnim() {}           // 0x45bfc0

void BirdSpySingleAnim::load() {                     // v15 0x45c070
    std::string n = std::string(name) + k_dir[dir] + "_";
    anim = Gorilla::gnew<Animation>(n.c_str());
    anim->load();
    anim->setHit(1, true);
    frame = 0;
}

void BirdSpySingleAnim::unload() {                   // v16 0x45c200
    Graphic::unload();
    if (anim) {
        anim->unload();
        delete anim;
        anim = nullptr;
    }
}

void BirdSpySingleAnim::draw() {                     // v14 0x45c240
    anim->x = x;
    anim->y = y;
    anim->setFrame(frame);
    anim->draw();
}

// ---------------------------------------------------------------- BirdSpy8Anim
BirdSpy8Anim::BirdSpy8Anim(const char *n) : Graphic(GRAPHIC_ARGS) {
    strncpy(name, n, sizeof name - 1);
    name[sizeof name - 1] = 0;
    setZ(50);
}
BirdSpy8Anim::~BirdSpy8Anim() {}

void BirdSpy8Anim::load() {                          // v15 0x45c630
    for (int i = 0; i < 8; i++) {
        std::string n = std::string(name) + k_dir[i] + "_";
        anims[i] = Gorilla::gnew<Animation>(n.c_str());
        anims[i]->load();
        anims[i]->setHit(1, true);
    }
    dir = 0;
    frame = 0;
}

void BirdSpy8Anim::unload() {                        // v16 0x45c7e0
    Graphic::unload();
    for (int i = 0; i < 8; i++) {
        if (!anims[i]) continue;
        anims[i]->unload();
        delete anims[i];
        anims[i] = nullptr;
    }
}

void BirdSpy8Anim::draw() {                          // v14 0x45c820
    anims[dir]->x = x;
    anims[dir]->y = y;
    anims[dir]->setFrame(frame);
    anims[dir]->draw();
}

// ---------------------------------------------------------------- BirdSpyGeneral / BirdSpy8 / BirdSpyCloud
BirdSpyGeneral::BirdSpyGeneral() : Graphic(GRAPHIC_ARGS) {   // 0x45c280
    setZ(50);
    x = 0;
    y = 0;
}

// v14 0x45c3d0: the first of its flights going its way, at its spot and frame
void BirdSpyGeneral::draw() {
    for (int i = 0; i < count; i++) {
        BirdSpySingleAnim *a = list[i];
        if (a->dir != dir) continue;
        a->x = x;
        a->y = y;
        a->frame = frame;
        a->draw();
        return;
    }
}

// 0x45c440: the same flight's frame hit at x, y. port: the exe also makes (and drops) an unused Image there.
bool BirdSpyGeneral::hitAt(int px, int py) {
    for (int i = 0; i < count; i++) {
        BirdSpySingleAnim *a = list[i];
        if (a->dir != dir) continue;
        a->x = x;
        a->y = y;
        a->frame = frame;
        a->anim->x = a->x;
        a->anim->y = a->y;
        a->anim->setFrame(a->frame);
        return a->anim->hit(px, py);
    }
    return false;
}

BirdSpy8::BirdSpy8(BirdSpy8Anim *a) : Graphic(GRAPHIC_ARGS), anim8(a) {
    x = 0;
    y = 0;
    setZ(50);
}

void BirdSpy8::draw() {                              // v14 0x45c900
    anim8->x = x;
    anim8->y = y;
    anim8->dir = dir;
    anim8->frame = frame;
    anim8->draw();
}

BirdSpyCloud::BirdSpyCloud(Image *i) : Graphic(GRAPHIC_ARGS), img(i) {}

void BirdSpyCloud::draw() {
    img->x = x;
    img->y = y;
    img->draw();
}

// ---------------------------------------------------------------- BirdCursor
BirdCursor::BirdCursor()                             // 0x45f100
    : Graphic(GRAPHIC_ARGS),
      active("bird_cu_active_01", 0, 0, 0, 0, 0, 0, true),
      lensImg("bird_cu_lens_01", 0, 0, 0, 0, 0, 0, true),
      notactive("bird_cu_notactive_01", 0, 0, 0, 0, 0, 0, true),
      hand_anim("glob_cu_active_"),
      lens(0, 0, 5, 38) {}

void BirdCursor::load() {                            // v15 0x45f2e0
    on = false;
    hand = false;
    active.load();
    lensImg.load();
    notactive.load();
    lensImg.translucency = 0x80;                     // +0x1b4
    hand_anim.load();
    hand_anim.mode = 1;                              // +0x394
    hand_t = frame_time();
    t = frame_time();
}

void BirdCursor::unload() {                          // v16 0x45f360
    Graphic::unload();
    active.unload();
    lensImg.unload();
    notactive.unload();
    hand_anim.unload();
}

// v14 0x45f3b0: over the map icon, the hand (a step every 50 ms); else the glass: what is under it doubled, its
// rim, and the cross-hair lit when a bird is under it.
void BirdCursor::draw() {
    if (hand) {
        uint32_t now = frame_time();
        if (now - hand_t > 50) {                     // 0x32
            hand_t = now;
            hand_anim.step();
        }
        hand_anim.x = x;
        hand_anim.y = y;
        hand_anim.draw();
        return;
    }
    lens.x = x;
    lens.y = y;
    lens.draw();
    lensImg.x = x;
    lensImg.y = y;
    lensImg.draw();
    if (on) {
        active.x = x;
        active.y = y;
        active.draw();
    } else {
        notactive.x = x;
        notactive.y = y;
        notactive.draw();
    }
}

// ---------------------------------------------------------------- BirdPeterRenderer
BirdPeterRenderer::BirdPeterRenderer()               // 0x45c950
    : Graphic(GRAPHIC_ARGS),
      fly_no("bird_ca_petflyno_"), fly_so("bird_ca_petflyso_"), fly_ea("bird_ca_petflyea_"),
      fly_we("bird_ca_petflywe_"), idle("bird_ca_petflyidle_"),
      str_no("bird_ca_petstrno_"), str_so("bird_ca_petstrso_"), str_ea("bird_ca_petstrea_"),
      str_we("bird_ca_petstrwe_") {}

void BirdPeterRenderer::load() {                     // v15 0x45cbb0
    x = 0;
    y = 0;
    setZ(20);                                        // 0x14
    for (Animation *a : {&fly_no, &fly_so, &fly_ea, &fly_we, &idle}) {
        a->load();
        a->mode = 2;
        a->setHit(1, true);
    }
    for (Animation *a : {&str_no, &str_so, &str_ea, &str_we}) {
        a->load();
        a->setHit(1, true);
    }
    cur = &idle;
    dir = 666;                                       // 0x29a
    lastdir = 666;
}

void BirdPeterRenderer::unload() {                   // v16 0x45cd00
    detach();
    Graphic::unload();
    for (Animation *a : {&fly_no, &fly_so, &fly_ea, &fly_we, &idle, &str_no, &str_so, &str_ea, &str_we}) a->unload();
}

// 0x45cda0: on along the same way; else into the new way through the take-off ("str") frames: a flight turns
// back down its own take-off, a take-off runs backwards to its start and then forwards into the new one.
void BirdPeterRenderer::animate() {
    if (dir == lastdir) {
        cur->step();
        return;
    }
    Animation *str = nullptr, *fly = nullptr;
    switch (dir) {
    case 0: str = &str_no; fly = &fly_no; break;
    case 2: str = &str_ea; fly = &fly_ea; break;
    case 4: str = &str_so; fly = &fly_so; break;
    case 6: str = &str_we; fly = &fly_we; break;
    case 666: fly = &idle; break;
    }
    if (cur == fly) {
        cur->step();
        lastdir = dir;
        return;
    }
    if (cur == str) {
        cur->step();
        if (cur->current() >= cur->count() - 1) {
            cur = fly;
            fly->setFrame(0);
            lastdir = dir;
        }
        return;
    }
    if (cur == &idle) {
        cur = str;
        str->setFrame(0);
        return;
    }
    Animation *back = nullptr;
    if (cur == &fly_no) back = &str_no;
    else if (cur == &fly_so) back = &str_so;
    else if (cur == &fly_ea) back = &str_ea;
    else if (cur == &fly_we) back = &str_we;
    if (back) {
        cur = back;
        cur->setFrame(cur->count() - 1);
        return;
    }
    if (cur->current() != 0) {
        cur->setFrame(cur->current() - 1);
        return;
    }
    if (dir == 666) {
        cur = &idle;
        idle.setFrame(0);
    } else {
        cur = str;
        str->setFrame(0);
    }
}

void BirdPeterRenderer::draw() {                     // v14 0x45cf60
    uint32_t now = frame_time();
    if (now - timer >= 30) {                         // 0x1e
        timer = now;
        animate();
    }
    cur->x = x;
    cur->y = y;
    cur->draw();
}

// ---------------------------------------------------------------- BirdPeterControlTask
BirdPeterControlTask::BirdPeterControlTask() {}
BirdPeterControlTask::~BirdPeterControlTask() {      // 0x45cfd0
    if (renderer) {
        renderer->detach();
        delete renderer;
    }
}

// 0x45d050: his way at his speed, and with the ground as it slides (one a step) and scrolls
void BirdPeterControlTask::move() {
    BirdSpy_Scene &s = S();
    int dx = 0;
    if (s.left && s.ground->x < 64) dx = 1;
    if (s.right && s.ground->x > -64) dx = -1;
    if (dir == 1 || dir == 2 || dir == 3) dx += speed;
    if (dir == 7 || dir == 6 || dir == 5) dx -= speed;
    x += dx;
    int dy;
    if (s.down) dy = s.up ? 0 : -1;
    else dy = s.up ? 1 : 0;
    if (dir == 7 || dir == 0 || dir == 1) dy -= speed;
    if (dir == 5 || dir == 4 || dir == 3) dy += speed;
    y += dy;
}

// 0x45d120: back in from the edges
void BirdPeterControlTask::bounds() {
    if (moving) return;
    counter = 0;
    duration = vision_random(30) + 5;
    if (x > 220) { moving = true; dir = 6; }
    if (x < -220) { moving = true; dir = 2; }
    if (y < -100) { moving = true; dir = 4; }
    if (y > 100) { moving = true; dir = 0; }
}

// 0x45d1d0: off somewhere: toward the middle, or any of the four ways
void BirdPeterControlTask::wander() {
    if (moving) return;
    counter = 0;
    duration = vision_random(30) + 5;
    moving = true;
    if (x > 120) dir = 6;
    else if (x < -120) dir = 2;
    else if (y < -100) dir = 4;
    else if (y > 100) dir = 0;
    else {
        switch (vision_random(4)) {
        case 0: dir = 0; break;
        case 1: dir = 4; break;
        case 2: dir = 2; break;
        case 3: dir = 6; break;
        }
    }
}

// 0x45d290: after the nearest bird he has not spotted yet, when it is near and on the screen
void BirdPeterControlTask::chase() {
    if (moving) return;
    int bx = 0, by = 0;
    int d = S().nearest(x, y, &bx, &by);
    if (d == -1 || d > 700) return;                  // 0x2bc
    if (d < 30) {
        counter = 0;
        duration = 10;
        dir = 666;
        return;
    }
    if (abs(bx) > 200 || abs(by) > 150) return;
    moving = true;
    counter = 0;
    duration = (2 * (d / speed)) / 3;
    if (abs(x - bx) > abs(y - by)) dir = x > bx ? 6 : 2;
    else dir = y > by ? 0 : 4;
}

// 0x45d3e0: every 3 seconds at most, a look just ahead of him (and around there, more tries the harder the game);
// a bird he has already got is taken again only now and then, and never for the tenth icon.
void BirdPeterControlTask::spot() {
    BirdSpy_Scene &s = S();
    uint32_t now = frame_time();
    if (now - lastspot < 3000) return;               // 0xbb8
    int ox = 0, oy = 0;                              // port: for other ways the exe reads unset locals (never happens)
    switch (dir) {
    case 2: ox = 30; oy = -30; break;
    case 0: ox = 0; oy = -40; break;
    case 4: ox = 0; oy = -10; break;
    case 6: ox = -30; oy = -30; break;
    case 666: ox = 0; oy = -25; break;
    }
    int px = x + ox, py = y + oy;
    char name[0x28];
    int i = s.pick(px, py, name);
    int tries = 0;                                   // port: unset in the exe for a skill outside 1..3
    switch (Game::state().skill) {
    case 1: tries = 5; break;
    case 2: tries = 15; break;
    case 3: tries = 30; break;
    }
    for (int t = 1; i == -1 && t < tries; t++) {
        int rx = vision_random(80);
        int ry = vision_random(80);
        i = s.pick(px + rx - 40, py + ry - 40, name);
    }
    if (i == -1) return;
    int idx = s.birdIndex(name);
    if (s.score->peterHas(idx)) {
        if (s.score->peter.size() == 9) return;
        int r = 1;
        switch (Game::state().skill) {
        case 1: r = vision_random(10); break;
        case 2: r = vision_random(30); break;
        case 3: r = vision_random(50); break;
        }
        if (r != 0) return;
    }
    if (!stop) {
        s.score->addPeter(idx);
        lastspot = frame_time();
    }
}

// 0x45d680: until the game is over: show him, move, turn now and then, chase, look; then off the bottom.
void BirdPeterControlTask::run() {
    wait(1);
    do {
        renderer->x = x;
        renderer->y = y;
        renderer->dir = dir;
        move();
        if (moving && ++counter >= duration) {
            moving = false;
            dir = 666;
        }
        bounds();
        if (vision_random(30) == 0) wander();
        int k = 0;
        switch (Game::state().skill) {
        case 1: k = 5; break;
        case 2: k = 3; break;
        case 3: k = 1; break;
        }
        if (vision_random(k * 3) == 0) chase();
        spot();
        wait(30);
    } while (!S().over);                             // 0x5c7c80
    renderer->dir = 4;
    while (y < 400) {                                // 0x190
        y += 8;
        renderer->x = x;
        renderer->y = y;
        wait(30);
    }
    for (;;) wait(1000);
}

// ---------------------------------------------------------------- the birds
// The start spot by the way it flies, from off the screen (0x45d890)
static const int k_start[8][2] = {{0, 300}, {-400, 100}, {-400, -200}, {-400, -500},
                                  {0, -300}, {400, -500}, {400, -200}, {400, 100}};

// 0x457cb0: the wing sound by the bird: 0 the hummingbird's, 1 large, 2 medium, 3 small
static int sound_index(const char *n) {
    if (!strcmp(n, "hummingbird")) return 0;
    if (!strcmp(n, "crow")) return 2;
    if (!strcmp(n, "parrot")) return 3;
    if (!strcmp(n, "sparrow")) return 3;
    if (!strcmp(n, "toucan")) return 2;
    if (!strcmp(n, "pelican")) return 2;
    if (!strcmp(n, "owl")) return 1;
    if (!strcmp(n, "flamingo")) return 1;
    if (!strcmp(n, "hawk")) return 1;
    if (!strcmp(n, "crane")) return 1;
    return 1;
}
static const char *const k_wing[4] = {"BIRD_FX_HUMfly", "BIRD_FX_lgwing", "BIRD_FX_mdwing", "BIRD_FX_smwing"};   // 0x539760

static int bird_z(const char *n) {                   // 50 + 20 by the bird (the tables 0x45ebb0 / the General's)
    int i = S().birdIndex(n);
    return (unsigned)i <= 9 ? 50 + 20 * i : -1;
}

// 0x45d890: a big bird of the kind: its flights, speed (by the skill), a way at random, a start off the screen that
// way (and up to 100 off), its depth, its wing sound.
GeneralBirdTask::GeneralBirdTask(Screen *scr, const char *name) : sound(nullptr) {
    BirdSpy_Scene &s = S();
    g = Gorilla::gnew<BirdSpyGeneral>();
    auto set = [&](std::initializer_list<BirdSpySingleAnim *> l, int k) {
        g->count = 0;
        for (BirdSpySingleAnim *a : l) g->list[g->count++] = a;
        speed = ftol((double)s.mult * (double)k);
        strncpy(g->name, name, sizeof g->name - 1);
    };
    if (!strcmp(name, "pelican")) set({&s.pel[0], &s.pel[1], &s.pel[2], &s.pel[3]}, 5);
    if (!strcmp(name, "owl")) set({&s.owl[0], &s.owl[1], &s.owl[2], &s.owl[3]}, 7);
    if (!strcmp(name, "hawk")) set({&s.haw[0], &s.haw[1], &s.haw[2], &s.haw[3]}, 7);
    if (!strcmp(name, "flamingo")) set({&s.fla[0], &s.fla[1]}, 5);
    if (!strcmp(name, "crane")) set({&s.cra[0], &s.cra[1]}, 5);
    t0 = frame_time();
    int r = vision_random(g->count);
    g->dir = g->list[r]->dir;
    int ox = vision_random(200) - 100;
    int oy = vision_random(200) - 100;
    g->x = k_start[g->dir][0] + ox;
    g->y = k_start[g->dir][1] + oy;
    g->setZ(bird_z(name));
    scr->layer(0)->add(g);
    Task::activate();
    soundIdx = sound_index(name);
    sound.setName(k_wing[soundIdx]);
    sound.open();
}

GeneralBirdTask::~GeneralBirdTask() {                // 0x45e030
    sound.close();
    g->detach();
    S().zRemove(g);
    delete g;
}

// The step of a bird over the ground (0x45e120 / 0x45ecf0): its way at its speed, plus the ground's slide and scroll.
// port: on the screen the exe adds the speed through the FPU (ftol(speed + v)), off it as integers: the same sums.
static void bird_step(Graphic *g, int d, int speed) {
    BirdSpy_Scene &s = S();
    int dx = ground_dx(3);
    if (d == 1 || d == 2 || d == 3) dx += speed;
    if (d == 7 || d == 6 || d == 5) dx -= speed;
    g->x += dx;
    int dy;
    if (s.down) dy = s.up ? 2 : 1;                   // 0x5c7c71 / 0x5c7c70
    else dy = s.up ? 4 : 2;
    if (d == 7 || d == 0 || d == 1) dy -= speed;
    if (d == 5 || d == 4 || d == 3) dy += speed;
    g->y += dy;
}

// The wings: frames 0..5 back and forth
static void flap(int &frame, int &step) {
    frame += step;
    if (frame < 0) {
        frame = 1;
        step = 1;
    } else if (frame >= 6) {
        frame = 4;
        step = -1;
    }
}

// 0x45e120: every 30 ms: flap (the sound on frame 0), fly; gone once well off the screen.
// The exe's time test, (start - now) < 10 s unsigned, only holds on the first step: kept.
void GeneralBirdTask::run() {
    wait(1);
    for (;;) {
        wait(30);
        flap(g->frame, g->step);
        if (g->frame == 0) sound.play();
        bird_step(g, g->dir, speed);
        if ((uint32_t)(t0 - frame_time()) < 10000) continue;   // 0x2710
        if (g->x >= -500 && g->x <= 500 && g->y >= -400 && g->y <= 400) continue;
        break;
    }
    finish();
}

// 0x45e3f0: a small bird: its eight flights, speed and turning, a start off one edge (top twice as likely) going
// in, its depth, its wing sound (the hummingbird's hums all the time).
Bird8Task::Bird8Task(Screen *scr, const char *name) : sound(nullptr) {
    BirdSpy_Scene &s = S();
    auto set = [&](BirdSpy8Anim *a, int k, int tc) {
        g = Gorilla::gnew<BirdSpy8>(a);
        strncpy(g->name, name, sizeof g->name - 1);
        speed = ftol((double)s.mult * (double)k);
        turn = tc;
    };
    if (!strcmp(name, "hummingbird")) set(&s.hum, 5, 10);
    if (!strcmp(name, "crow")) set(&s.cro, 7, 25);
    if (!strcmp(name, "parrot")) set(&s.par, 4, 20);
    if (!strcmp(name, "sparrow")) set(&s.spa, 5, 15);
    if (!strcmp(name, "toucan")) set(&s.tou, 4, 50);
    t0 = frame_time();
    switch (vision_random(100) / 20) {
    case 0:
    case 1:
        g->x = vision_random(500) - 250;
        g->y = -400;
        g->dir = 4;
        break;
    case 2:
        g->x = vision_random(500) - 250;
        g->y = 400;
        g->dir = 0;
        break;
    case 3:
        g->x = -500;
        g->y = vision_random(100) - 240;
        g->dir = 2;
        break;
    case 4:
        g->x = 500;
        g->y = vision_random(100) - 240;
        g->dir = 6;
        break;
    }
    g->setZ(bird_z(name));
    entered = false;
    scr->layer(0)->add(g);
    Task::activate();
    soundIdx = sound_index(name);
    sound.setName(k_wing[soundIdx]);
    if (soundIdx == 0) sound.type = 6;               // +0x41c: loops
    sound.open();
    if (soundIdx == 0) sound.play();
}

Bird8Task::~Bird8Task() {                            // 0x45ec00
    sound.close();
    g->detach();
    S().zRemove(g);
    delete g;
}

// 0x45ecf0: as the big birds', and once it has been on the screen it turns one way or the other now and then
void Bird8Task::run() {
    wait(1);
    for (;;) {
        wait(30);
        flap(g->frame, g->step);
        if (soundIdx && g->frame == 0) sound.play();
        if (entered) {
            if (!S().over && vision_random(turn) == 0) {
                if (vision_random(100) < 50) {
                    if (--g->dir < 0) g->dir = 7;
                } else {
                    if (++g->dir > 7) g->dir = 0;
                }
            }
        } else if (g->x >= -256 && g->x <= 256 && g->y >= -240 && g->y <= 240) {
            entered = true;
        }
        bird_step(g, g->dir, speed);
        if ((uint32_t)(t0 - frame_time()) < 10000) continue;
        if (g->x >= -500 && g->x <= 500 && g->y >= -400 && g->y <= 400) continue;
        break;
    }
    finish();
}

// ---------------------------------------------------------------- CloudTask
// 0x45f490: a cloud across the top at random; small mostly, else (by the exe's test) always the large one
CloudTask::CloudTask(Screen *scr) {
    BirdSpy_Scene &s = S();
    int a = 0, b = 0;
    switch (Game::state().skill) {
    case 1: a = 80; b = 20; break;
    case 2: a = 75; b = 20; break;
    case 3: a = 70; b = 25; break;
    }
    int r = vision_random(100);
    Image *img;
    if (r <= a) img = &s.cloudsm;
    else {
        int k = r <= b ? 2 : 1;                      // r > a >= b here: always 1
        img = k == 2 ? &s.cloudmd : &s.cloudlg;
    }
    g = Gorilla::gnew<BirdSpyCloud>(img);
    g->x = vision_random(640) - 320;
    g->y = -500;
    g->setZ(20);
    s.zAdd(g);
    scr->layer(0)->add(g);
}

CloudTask::~CloudTask() {                            // 0x45f6b0
    g->detach();
    S().zRemove(g);
    delete g;
}

void CloudTask::run() {                              // 0x45f780: down the screen with the ground, until gone
    BirdSpy_Scene &s = S();
    wait(1);
    do {
        wait(30);
        int dy = s.down ? 2 : (s.up ? 6 : 4);
        int dx;
        if (s.right) dx = s.ground->x > -64 ? -4 : 0;
        else dx = (s.left && s.ground->x < 64) ? 4 : 0;
        g->x += dx;
        g->y += dy;
    } while (g->y <= 700);                           // 0x2bc
    finish();
}

// ---------------------------------------------------------------- BirdScore
BirdScore::BirdScore() : Graphic(GRAPHIC_ARGS), rings("BIRD_IF_rings_01", 0, 0, 0, 0, 0, 0, true) {   // 0x45f880
    rings.load();
}

BirdScore::~BirdScore() {                            // 0x45f970
    rings.unload();
    for (Image *i : player) { i->unload(); delete i; }
    for (Image *i : peter) { i->unload(); delete i; }
}

void BirdScore::draw() {                             // v14 0x45fb80
    rings.draw();
    for (size_t i = 0; i < player.size(); i++) player.at(i)->draw();
    for (size_t i = 0; i < peter.size(); i++) peter.at(i)->draw();
}

static bool has_icon(const std::vector<Image *> &v, const std::string &n) {
    for (Image *i : v)
        if (!strcasecmp(i->name(), n.c_str())) return true;
    return false;
}

// 0x45fc20: the bird's icon down the player's side (two columns); a bird already there counts once, its icon dark
void BirdScore::add(int idx) {
    if (player.size() >= 10) return;
    std::string n;
    if (!has_icon(player, small_name(idx))) {
        p++;
        n = small_name(idx);
    } else {
        n = smalldk_name(idx);
    }
    int k = (int)player.size();
    Image *im = Gorilla::gnew<Image>(n.c_str(), 0, 0, 0, 0, 0, 0, true);
    im->load();
    im->x = (k & 1) ? -249 : -292;
    im->y = 29 * k - 128;
    player.push_back(im);
}

void BirdScore::addPeter(int idx) {                  // 0x45ff50: the same on Peter's side
    if (peter.size() >= 10) return;
    std::string n;
    if (!has_icon(peter, small_name(idx))) {
        s++;
        n = small_name(idx);
    } else {
        n = smalldk_name(idx);
    }
    int k = (int)peter.size();
    Image *im = Gorilla::gnew<Image>(n.c_str(), 0, 0, 0, 0, 0, 0, true);
    im->load();
    im->x = (k & 1) ? 252 : 295;
    im->y = 29 * k - 128;
    peter.push_back(im);
}

bool BirdScore::peterHas(int idx) { return has_icon(peter, small_name(idx)); }   // 0x460280

// ---------------------------------------------------------------- BirdSpy_Scene
BirdSpy_Scene::BirdSpy_Scene()                       // 0x459f50
    : Game2D_Scene("birdspy"),
      guide("BIRD_VO_PETgreet1", "BIRD_VO_PETgreet2"),
      hum("bird_ga_humfly"), cro("bird_ga_crofly"), par("bird_ga_parfly"), spa("bird_ga_spafly"), tou("bird_ga_toufly"),
      pel{{"bird_ga_pelfly", 1}, {"bird_ga_pelfly", 7}, {"bird_ga_pelfly", 3}, {"bird_ga_pelfly", 5}},
      owl{{"bird_ga_owlfly", 0}, {"bird_ga_owlfly", 7}, {"bird_ga_owlfly", 3}, {"bird_ga_owlfly", 4}},
      haw{{"bird_ga_hawfly", 0}, {"bird_ga_hawfly", 2}, {"bird_ga_hawfly", 6}, {"bird_ga_hawfly", 4}},
      fla{{"bird_ga_flafly", 2}, {"bird_ga_flafly", 6}},
      cra{{"bird_ga_crafly", 1}, {"bird_ga_crafly", 5}},
      cloudlg("bird_ga_cloudlg_01", 0, 0, 0, 0, 0, 0, true),
      cloudmd("bird_ga_cloudmd_01", 0, 0, 0, 0, 0, 0, true),
      cloudsm("bird_ga_cloudsm_01", 0, 0, 0, 0, 0, 0, true),
      music(nullptr) {
    // port: the 30 icons are members in the exe (+0x2750.. 0xa8 apart); here made with the scene
    static const char *const lc[10] = {"cra", "cro", "fla", "haw", "hum", "owl", "par", "pel", "spa", "tou"};
    for (int i = 0; i < 10; i++) {
        small[i] = new Image((std::string("bird_if_") + lc[i] + "small_01").c_str(), 0, 0, 0, 0, 0, 0, true);
        bigdk[i] = new Image((std::string("bird_if_") + lc[i] + "bigdk_01").c_str(), 0, 0, 0, 0, 0, 0, true);
        biglt[i] = new Image((std::string("bird_if_") + lc[i] + "biglt_01").c_str(), 0, 0, 0, 0, 0, 0, true);
    }
    keyboard.setClient(this);
    mouse.setClient(this);
}

BirdSpy_Scene::~BirdSpy_Scene() {}                   // 0x45b070

// 0x459730
int BirdSpy_Scene::birdIndex(const char *n) {
    static const char *const names[10] = {"crane", "crow", "flamingo", "hawk", "hummingbird",
                                          "owl", "parrot", "pelican", "sparrow", "toucan"};
    for (int i = 0; i < 10; i++)
        if (!strcmp(n, names[i])) return i;
    return -1;
}

// The depth made one more than any bird's or cloud's it equals (one pass, in order), and the graphic listed
void BirdSpy_Scene::zAdd(Graphic *g) {
    int z = g->z;
    for (int i = 0; i < zcount; i++)
        if (z == zslots[i]->z) z++;
    g->setZ(z);
    zslots[zcount] = g;
    zcount++;
}

void BirdSpy_Scene::zRemove(Graphic *g) {
    int i = 0;
    for (; i < zcount; i++)
        if (zslots[i] == g) break;
    if (i >= zcount) return;
    for (; i < zcount - 1; i++) zslots[i] = zslots[i + 1];
    zcount--;
}

// 0x459140: any of a 5 x 5 grid 10 apart around x, y on the bird's frame
bool BirdSpy_Scene::hitNear(Graphic *g, int x, int y) {
    for (int j = 0; j < 5; j++) {
        int py = y - 20 + 10 * j;
        for (int i = 0; i < 5; i++) {
            int px = x - 20 + 10 * i;
            if (BirdSpyGeneral *b = dynamic_cast<BirdSpyGeneral *>(g)) {
                if (b->hitAt(px, py)) return true;
            } else {
                BirdSpy8 *e = (BirdSpy8 *)g;
                BirdSpy8Anim *a = e->anim8;
                a->x = e->x;
                a->y = e->y;
                a->dir = e->dir;
                a->frame = e->frame;
                Animation *an = a->anims[a->dir];
                an->x = a->x;
                an->y = a->y;
                an->setFrame(a->frame);
                if (an->hit(px, py)) return true;
            }
        }
    }
    return false;
}

// 0x459410: the bird nearest the front at x, y (a cloud in front hides it); on a click each bird hit counts once.
int BirdSpy_Scene::pick(int x, int y, char *name) {
    if (x < -256 || x > 256) return -1;
    int best = 100000000;                            // 0x5f5e100
    int result = -1;
    for (int i = 0; i < zcount; i++) {
        Graphic *g = zslots[i];
        if (BirdSpyGeneral *b = dynamic_cast<BirdSpyGeneral *>(g)) {
            if (hitNear(b, x, y) && b->z < best) {
                if (clicking) {
                    if (b->caught) continue;
                    b->caught = true;
                }
                result = i;
                best = b->z;
                strcpy(name, b->name);
            }
        } else if (BirdSpy8 *e = dynamic_cast<BirdSpy8 *>(g)) {
            if (hitNear(e, x, y) && e->z < best) {
                if (clicking) {
                    if (e->caught) continue;
                    e->caught = true;
                }
                result = i;
                best = e->z;
                strcpy(name, e->name);
            }
        } else if (BirdSpyCloud *c = dynamic_cast<BirdSpyCloud *>(g)) {
            c->img->x = c->x;
            c->img->y = c->y;
            if (c->img->hit(x, y) && c->z < best) {
                result = -1;
                best = c->z;
            }
        } else if (Image *im = dynamic_cast<Image *>(g)) {
            if (im->hit(x, y) && im->z < best) {
                result = -1;
                best = im->z;
            }
        }
    }
    return result;
}

// 0x4592a0: the nearest bird Peter has not got (within sqrt(1e9)): its spot, and a distance.
// The exe returns the root of the last bird's squared distance it measured, not the nearest's: kept.
int BirdSpy_Scene::nearest(int x, int y, int *bx, int *by) {
    int best = 1000000000;                           // 0x3b9aca00
    int result = -1;
    int last = 0;
    for (int i = 0; i < zcount; i++) {
        Graphic *g = zslots[i];
        const char *n;
        if (BirdSpyGeneral *b = dynamic_cast<BirdSpyGeneral *>(g)) n = b->name;
        else if (BirdSpy8 *e = dynamic_cast<BirdSpy8 *>(g)) n = e->name;
        else continue;
        int dx = x - g->x, dy = y - g->y;
        int d2 = dx * dx + dy * dy;
        last = d2;
        if (d2 > best) continue;
        int idx = birdIndex(n);
        if (score->peterHas(idx)) continue;
        best = d2;
        result = idx;
        *bx = g->x;
        *by = g->y;
    }
    if (result == -1) return -1;
    return ftol(sqrt((double)last));
}

// v18 0x459680
void BirdSpy_Scene::load2() {
    leaves = 0;
    inactivity.add("BIRD_VO_PETprompt", false);
    endvo.setWins("BIRD_VO_PETtbwins_01", "BIRD_VO_PETtbwins_02", "BIRD_VO_PETtbwins_03");
    endvo.setLoses("BIRD_VO_PETtblose_01", "BIRD_VO_PETtblose_02", "BIRD_VO_PETtblose_03");
    endvo.setTie("BIRD_VO_PETtie_01");
    endvo.setWinChallenge("BIRD_VO_PETwinsch_01", "BIRD_VO_PETwinsch_02", "BIRD_VO_PETwinsch_03");
    endvo.setAward("BIRD_VO_PETaward");
    endvo.setLoseChallenge("BIRD_VO_PETlosech_01", "BIRD_VO_PETlosech_02", "BIRD_VO_PETlosech_03");
    endvo.setTie2("BIRD_VO_PETtie_02");
}

// v19 0x459840: input; the "bird_spy" pack; the glass cursor (hidden for now); the end scoreboard; Peter's guide
// (unless coming back to this game); the score rings.
void BirdSpy_Scene::enter() {
    Game2D_Scene::enter();
    over = false;
    mouse.activate();
    keyboard.activate();
    pack = ras_open("bird_spy", nullptr, &data_device());
    pack->load();
    ras_set_current(pack);
    screen.activate();
    screen.loadAll();
    cursor.load();                                   // v15
    cursor.visible = true;                           // +0x3b7c
    set_cursor(&cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
    // (the exe zeroes the cursor hotspot 0x5a6930/34 here; the port has none)
    hide_cursor();
    board = Gorilla::gnew<Scoreboard>("BIRD_IF_scorescr_01");
    board->setZ(-1000);
    board->visible = false;
    screen.layer(0)->add(board);
    zcount = 0;
    if (previous != this) {                          // +0xc4
        guide.load(false);                           // 0x4ceb60
        guide.setZ(-950);
        screen.layer(0)->add(&guide);
        guide.Task::activate();                      // +0xa40 v6
    }
    reset();                                         // 0x421110
    activate();
    playing = false;
    score = Gorilla::gnew<BirdScore>();              // 0x4243c0(0x168)
    score->setZ(-1000);
    screen.layer(0)->add(score);
}

// 0x45b8b0: the ground; Peter (his flight, at the left, hovering); everything's frames; the cannon; the music and
// the three birdcalls, each next due within two seconds.
void BirdSpy_Scene::setup() {
    ground = Gorilla::gnew<ScrollGround>("BIRD_BG_ground1_01", "BIRD_BG_ground2_01");
    ground->load();
    screen.layer(0)->add(ground);
    peter = Gorilla::gnew<BirdPeterControlTask>();
    peter->renderer = Gorilla::gnew<BirdPeterRenderer>();
    zAdd(peter->renderer);
    screen.layer(0)->add(peter->renderer);
    peter->speed = 7;
    peter->x = -100;
    peter->y = 100;
    peter->dir = 666;
    peter->moving = false;
    peter->renderer->load();
    peter->lastspot = frame_time();
    peter->stop = false;
    peter->activate();
    for (Image *c : {&cloudlg, &cloudmd, &cloudsm}) {
        c->load();
        c->setHit(1, true);
    }
    for (BirdSpy8Anim *a : {&hum, &cro, &par, &spa, &tou}) a->load();
    for (int i = 0; i < 4; i++) pel[i].load();
    for (int i = 0; i < 4; i++) owl[i].load();
    for (int i = 0; i < 4; i++) haw[i].load();
    for (int i = 0; i < 2; i++) fla[i].load();
    for (int i = 0; i < 2; i++) cra[i].load();
    for (int i = 0; i < 10; i++) small[i]->load();
    for (int i = 0; i < 10; i++) bigdk[i]->load();
    for (int i = 0; i < 10; i++) biglt[i]->load();
    Cannon &cn = Game::cannon();                     // state +0x7c0
    screen.layer(0)->add(&cn);
    cn.visible = true;                               // state +0x83c
    ground->visible = true;
    music.setName("BIRD_MU_music");                  // *0x539750
    music.mode = 2;                                  // +0x4c98
    music.type = 6;                                  // +0x4ca0
    music.open();
    music.play();
    static const char *const names[3] = {"BIRD_FX_caw", "BIRD_FX_chirp", "BIRD_FX_tweet"};   // 0x539754
    for (int i = 0; i < 3; i++) {
        calls[i].setName(names[i]);
        calls[i].open();
    }
    for (int i = 0; i < 3; i++) {
        Line &l = when[i];
        float v = (float)((double)now_secs() * l.a + l.b);
        l.a = -1.0f;
        l.b = (float)((double)v - (double)now_secs() * l.a);
        float r = (float)((double)crt_rand() * (double)3.05185e-05f);
        r = r + r;
        l.b = (float)((double)r - (double)now_secs() * l.a);
    }
    clicking = false;
}

// v10 0x458120
void BirdSpy_Scene::run() {
    State &st = Game::state();
    wait(1);
    hide_cursor();
    clouds->reset();
    dbg("run");
    while (guide.Task::active()) wait(0);            // +0xa40 v5
    setup();                                         // 0x45b8b0
    show_cursor();
    while (clouds->fall()) wait(30);
    VoiceOver::get().play("bird_vo_petchabeg_01");   // 0x4c9400
    switch (st.skill) {                              // +0x68
    case 1: secs = 90; mult = 0.8f; cloudKind = 2; cloudSkill = 1; break;
    case 2: secs = 90; mult = 1.0f; cloudKind = 3; cloudSkill = 2; break;
    case 3: secs = 60; mult = 1.2f; cloudKind = 4; cloudSkill = 3; break;
    }
    Game::cannon().start(secs);                      // 0x4e3a50
    inactivity.start();                              // v12
    playing = true;
    if (getenv("TB_DEBUG_BIRDSPY")) tb_log("birdspy: play, last_game %d", st.last_game);
    char name[0x28];
    for (;;) {
        wait(50);                                    // 0x32
        // the birdcalls, each at its time, the next one to five seconds on
        for (int i = 0; i < 3; i++) {
            Line &l = when[i];
            if ((double)now_secs() * l.a + l.b <= 0.0) {
                calls[i].play();
                float t = (float)((double)crt_rand() * (double)3.05185e-05f * 4.0 - 4.0 * 0.5 + 3.0);
                l.b = (float)((double)t - (double)now_secs() * l.a);
            }
        }
        if (map->hit(cursor.x, cursor.y)) {          // v11 at +0x3b58/+0x3b5c
            cursor.hand = true;
            cursor.on = false;
        } else {
            cursor.hand = false;
            cursor.on = frame_time() - last_spot >= 500 && pick(cursor.x, cursor.y, name) != -1;
        }
        // now and then a bird, at random of the ten
        if (vision_random(30) == 0) {                // the scene's random +0x1c8
            static const char *const small8[5] = {"hummingbird", "crow", "parrot", "sparrow", "toucan"};
            static const char *const big[5] = {"pelican", "owl", "hawk", "flamingo", "crane"};
            int r = vision_random(10);               // the table 0x459110
            Task *t;
            Graphic *g;
            if (r < 5) {
                Bird8Task *b = Gorilla::gnew<Bird8Task>(&screen, small8[r]);
                t = b;
                g = b->g;
            } else {
                GeneralBirdTask *b = Gorilla::gnew<GeneralBirdTask>(&screen, big[r - 5]);
                t = b;
                g = b->g;
            }
            zAdd(g);
            start(t);                                // 0x420ef0
        }
        // and a cloud: more often on the harder games, fewer scrolling slowly, more scrolling fast
        int n = (10 - cloudSkill) * 7;
        if (down) n = (n * 4) / 2;
        if (up) n = (n * 4) / 3;
        if (vision_random(n) == 0) start(Gorilla::gnew<CloudTask>(&screen));
        if (!Game::cannon().running()) break;
        if (st.last_game == 5) break;
        if (score->player.size() >= 10) break;
        if (score->peter.size() >= 10) break;
    }
    if (getenv("TB_DEBUG_BIRDSPY"))
        tb_log("birdspy: over: running %d last_game %d player %d peter %d (p %d s %d)", Game::cannon().running(),
               st.last_game, (int)score->player.size(), (int)score->peter.size(), score->p, score->s);
    hide_cursor();
    inactivity.stop();                               // v13
    peter->stop = true;
    if (st.last_game == 5) {                         // skipped: fade out and back to the village
        fader->visible = true;
        fader->start(false, 2000);
        while (fader->busy()) wait(0);
        change_scene("Indian_Village");              // 0x631700 (else 0x5a959c)
        return;
    }
    clouds->cover();
    while (clouds->rise()) wait(30);
    over = true;
    if (score->p > score->s) {
        int info = st.saves.gameInfo("Bird Spy Game");   // *0x5434fc
        if (info == 0) st.last_result = 2;
        else if (info == 1) st.last_result = 3;
        else if (info == 2) st.last_result = 4;
    } else if (score->p == score->s) {
        st.last_result = 5;
    } else if (score->p < score->s) {
        st.last_result = 1;
    }
    set_cursor(st.cursor, false);
    Game::cannon().hide();                           // 0x4e3bf0
    ground->setAuto(false);
    ground->visible = false;
    board->s2 = score->s;
    board->s1 = score->p;
    board->visible = true;
    wait(3000);
    endvo.playEnd();
    while (endvo.playing()) wait(0);
    if (score->p > score->s) {                       // (the static's score 0x5c7c74)
        change_scene("Indian_Village");
    } else {
        dialog = Gorilla::gnew<ContinueDialog>("glob_if_contgame_01", "glob_if_signglow_01", "glob_if_xglow_01");
        dialog->setAsk("bird_vo_petbegcon_01");
        dialog->setYes("bird_vo_petbegyes_01");
        dialog->setNo("bird_vo_petbegno_01");
        dialog->setIdle("bird_vo_petbegidl_01");
        keyboard.deactivate();                       // v7
        show_cursor();
        call(dialog);
        wait(1);
    }
    finish();
}

// v9 0x458060: the dialog's answer: again (the run restarted in place) or back to the village.
void BirdSpy_Scene::notify2(Service *s) {
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
            board->visible = false;
            again();                                 // 0x41dde0
        } else if (dialog->no) {                     // +0x6f0
            change_scene("Indian_Village");
        }
        dialog = nullptr;
        mouse.activate();
        keyboard.activate();
    }
}

// 0x457dc0: a left click: the map icon, else (twice a second at most) a spot of the bird under the glass
void BirdSpy_Scene::onMouse() {
    if (!g_cursor_graphic || !g_cursor_graphic->visible) return;
    if (!playing) return;
    if (!(g_mouse.ldown || g_mouse.rdown) || g_mouse.rbutton) return;   // 0x5a6e30, 0x5a6e6e
    int r = map->click(g_mouse.x, g_mouse.y, true);
    if (r) {
        if (r == 2) change_scene("map");
        return;
    }
    uint32_t now = frame_time();
    if (now - last_spot < 500) return;               // 0x1f4
    char name[0x28];
    clicking = true;
    int i = pick(cursor.x, cursor.y, name);
    clicking = false;
    if (i == -1) return;
    score->add(birdIndex(name));
    last_spot = now;
}

// 0x457eb0: Escape skips Peter's welcome, or the game; the arrows scroll and slide the ground (and the birds,
// clouds and Peter with it).
void BirdSpy_Scene::onKey() {
    State &st = Game::state();
    if (!playing) {
        if ((int)g_key.vk == st.skip_key) guide.stop();
        return;
    }
    ground->setMult(2);
    up = down = left = right = false;
    if (g_key.down) {                                // 0x5a6dd8
        if (st.key((int)g_key.vk)) return;
        if ((int)g_key.vk == st.skip_key) st.last_game = 5;
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

// v20 0x459a40
void BirdSpy_Scene::leave() {
    VoiceOver::get().stop();
    if (score) {
        score->detach();
        delete score;
    }
    // (the exe keeps the pointer: enter makes a new one)
    score = nullptr;
    Game2D_Scene::leave();
    guide.unload();                                  // v16
    music.close();
    for (int i = 0; i < 3; i++) calls[i].close();
    leaves++;
    Game::cannon().detach();
    // the birds and clouds still flying.
    // port: the exe deletes each child and then frees its node, which the child's dtor has already freed (when not
    // exiting); here the child lets go of the node first, so it is freed once. At the game's exit nodes are not
    // unlinked (0x4252e0), so the children are left to the process's end there.
    for (Node *c = tb_exiting() ? nullptr : children.first; c;) {
        Node *next = c->next;
        Task *t = dynamic_cast<Task *>(c->object());
        if (t) {
            if (t->finished_node == c) t->finished_node = nullptr;
            delete t;
        }
        c->remove();
        delete c;
        c = next;
    }
    deactivate();                                    // v7
    if (ground) {
        ground->unload();
        delete ground;
    }
    ground = nullptr;
    if (peter) {
        peter->deactivate();
        peter->renderer->unload();
        delete peter;
    }
    peter = nullptr;
    for (BirdSpy8Anim *a : {&hum, &cro, &par, &spa, &tou}) a->unload();
    for (int i = 0; i < 4; i++) pel[i].unload();
    for (int i = 0; i < 4; i++) owl[i].unload();
    for (int i = 0; i < 4; i++) haw[i].unload();
    for (int i = 0; i < 2; i++) fla[i].unload();
    for (int i = 0; i < 2; i++) cra[i].unload();
    for (Image *c : {&cloudlg, &cloudmd, &cloudsm}) c->unload();
    for (int i = 0; i < 10; i++) small[i]->unload();
    for (int i = 0; i < 10; i++) bigdk[i]->unload();
    for (int i = 0; i < 10; i++) biglt[i]->unload();
    delete board;
    board = nullptr;
    cursor.unload();
    mouse.deactivate();
    keyboard.deactivate();
    ras_release(pack);
    pack = nullptr;
    tasks_finish_all();                              // 0x4210d0
    screen.deactivate();
    screen.unloadAll();                              // (no clear here)
    set_cursor(Game::state().cursor, true);
    Game::state().guide_loaded = false;
}

}  // namespace Game
