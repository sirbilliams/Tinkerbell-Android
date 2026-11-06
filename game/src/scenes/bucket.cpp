// Bucket Toss, reconstructed from "Tinker Bell.exe" (see bucket.h).
#include "bucket.h"
#include "lost.h"
#include "state.h"
#include "cannon.h"
#include "tink3d.h"
#include "voiceover.h"
#include <string.h>

namespace Game {

using Raytrace::Attachment;
using Raytrace::Sprite3D;
using Vision::Animation;

static const char *const kGame = "Bucket Toss Game";   // *0x543534
static const char *const kOnTable[5] = {"blue_on_table", "green_on_table", "purple_on_table", "red_on_table",
                                        "yellow_on_table"};   // 0x536704..0x5366c4
static const char *const kSigns[5] = {"sign_blue", "sign_green", "sign_purple", "sign_red", "sign_yellow"};

static void say(const char *line) { VoiceOver::get().play(line); }   // 0x4c9400 on the state's +0x3e8
static Animation *anim_of(Attachment *a) { return static_cast<Animation *>(a->g); }

// ---------------------------------------------------------------- BucketCursor

BucketCursor::BucketCursor()                          // 0x43c9a0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {
    static const char *const names[5] = {"BUCK_CU_BALblu_", "BUCK_CU_BALgre_", "BUCK_CU_BALpur_",
                                         "BUCK_CU_BALred_", "BUCK_CU_BALyel_"};
    for (int i = 0; i < 5; i++) anims[i] = new Animation(names[i]);   // 0x4243c0(0x118) + 0x404c20
}

BucketCursor::~BucketCursor() {                       // 0x43cb50
    for (Animation *&a : anims) { delete a; a = nullptr; }   // v3 each
}

void BucketCursor::load() {                          // v15 0x43cc10
    if (loaded) return;
    loaded = true;
    for (Animation *a : anims) a->load();            // v15
    colour = 6;
}

void BucketCursor::unload() {                        // v16 0x43cc70
    Graphic::unload();                               // 0x412020
    if (!loaded) return;
    loaded = false;
    for (Animation *a : anims) a->unload();          // v16
}

void BucketCursor::draw() {                          // v14 0x43ccd0
    if (colour < 1 || colour > 5) return;
    Animation *a = anims[colour - 1];
    a->x = x;                                        // +0x58
    a->y = y;                                        // +0x5c
    a->draw();                                       // v14
}

// ---------------------------------------------------------------- BucketBall

BucketBall::BucketBall(Raytrace::RayTrace *r)         // 0x43cd40
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), rt(r) {
    for (Vision::Sound *&s : fx) s = new Vision::Sound("");   // 0x437290 x3 (named by load)
    static const char *const names[5] = {"BUCK_GA_thrblu_", "BUCK_GA_thrgre_", "BUCK_GA_thrpur_",
                                         "BUCK_GA_thrred_", "BUCK_GA_thryel_"};
    for (int i = 0; i < 5; i++) anims[i] = new Animation(names[i]);
    static const char *const lines[6] = {"BUCK_VO_RACnice1", "BUCK_VO_RACnice2", "BUCK_VO_RACnice3",
                                         "BUCK_VO_RACnice4", "BUCK_VO_RACnice5", "BUCK_VO_RACnice6"};
    for (const char *l : lines) nice.add(l);         // 0x4cc0d0
}

BucketBall::~BucketBall() {                          // 0x43cf80
    for (Animation *&a : anims) { delete a; a = nullptr; }
    for (Vision::Sound *&s : fx) { delete s; s = nullptr; }
}

void BucketBall::load() {                            // v15 0x43d070
    if (loaded) return;
    loaded = true;
    for (Animation *a : anims) a->load();
    colour = 6;
    flying = false;
    static const char *const names[3] = {"buck_fx_puzgoin", "BUCK_FX_woodknock", "buck_fx_puzthud"};   // 0x53646c
    for (int i = 0; i < 3; i++) {
        fx[i]->setName(names[i]);                    // 0x424400
        fx[i]->open();                               // 0x41f9d0(0)
    }
    wrong = low = highs = misses = 0;
    first = true;
}

void BucketBall::unload() {                          // v16 0x43d130
    Graphic::unload();
    if (!loaded) return;
    loaded = false;
    flying = false;
    for (Animation *a : anims) a->unload();
    for (Vision::Sound *s : fx) s->close();          // 0x41fc50
}

// 0x43d1b0: a wrong colour (not the sign's) is told every time (the counter is reset as soon as it reaches 1).
void BucketBall::toss(float px, float py, float camz, int col) {
    if (flying) return;
    flying = true;
    x = (int)px;                                     // 0x4f3948
    y = (int)py;
    colour = col;
    frame = 0;
    last = Vision::frame_time();
    high = right = left = false;
    target = (int)(((double)camz - 560.0f) * 0.0285714287f);   // 0x514218, 0x514214
    int n = anims[0]->count();                       // the blue one's frames, whatever the colour
    if (target >= n && target <= n + 2) target = n - 1;
    BucketGame *g = lost_scene().bucket;             // 0x5af938
    if (colour != g->sign_colour) {
        if (++wrong == 1) say("BUCK_VO_RACcolor_y");
    }
    wrong = 0;
}

// v14 0x43d2c0: every 60 ms a frame further; on the frame that reaches the tree, the bucket's stickers are tried
// where the piece is (the blue animation's frame offsets, 26 up).
void BucketBall::draw() {
    if (!flying) return;
    if (colour == 6 || colour == 0) {
        flying = false;
        return;
    }
    if (Vision::frame_time() - last > 0x3c) {
        last = Vision::frame_time();
        frame++;
        if (frame >= anims[0]->count() - 1) flying = false;
        if (frame == target) {
            Sprite3D *tree = rt->findSprite("bucket_tree");   // 0x42dc70
            tree->showAttached("bucket_hit");        // 0x42f690
            tree->showAttached("bucket_left");
            tree->showAttached("bucket_right");
            tree->showAttached("bucket_high");
            Vision::Image *f = anims[0]->frame(frame);   // 0x403ec0
            int hx = f->ox + x;                      // +0x30
            Vision::Image *f2 = anims[0]->frame(frame);
            int hy = f2->oy + y - 0x1a;              // +0x34
            Attachment *a = rt->hitAttached(tree, hx, hy, false);   // 0x42ca00
            Lost_Scene &sc = lost_scene();
            if (!a) {
                fx[2]->play();                       // thud
                low = highs = 0;
                misses++;
            } else if (a->name == "bucket_hit") {   // 0x42e5b0
                flying = false;
                BucketGame *g = sc.bucket;
                int col = colour;
                if (col == g->sign_colour) {
                    sc.hits++;                       // 0x5af940
                    g->fx[0]->play();                // signspin
                    g->sign_colour = 0;
                    int r = Vision::vision_random(300);   // 0x425d40 on the state's +8
                    g->spin_until = r + Vision::frame_time() + 300;
                    if (col >= 1 && col <= 5) g->done[col - 1] = true;
                    fx[0]->play();                   // puzgoin
                    low = highs = misses = 0;
                    if (first) {
                        say("BUCK_VO_RAC1stone");
                        first = false;
                    } else if (sc.hits < 5) {
                        nice.playNext();             // 0x4cc0e0
                    }
                } else {
                    fx[1]->play();                   // woodknock
                    low = highs = misses = 0;
                }
            } else {
                if (a->name == "bucket_left") left = true;
                else if (a->name == "bucket_right") right = true;
                else if (a->name == "bucket_high") high = true;
                if (left || right) {
                    fx[1]->play();
                    if (++low == 1) say("BUCK_VO_RAClow");
                    low = highs = misses = 0;
                } else if (high) {
                    if (++highs == 1) say("BUCK_VO_RAChigh");
                    low = highs = misses = 0;
                } else {
                    fx[2]->play();
                    low = highs = 0;
                    misses++;
                }
            }
            tree->hideAttached("bucket_hit");        // 0x42f620
            tree->hideAttached("bucket_left");
            tree->hideAttached("bucket_right");
            tree->hideAttached("bucket_high");
            if (misses == 2) say("BUCK_VO_RACremind_y");
            else if (misses == 3) say("BUCK_VO_RACencour");
        }
    }
    if (left) x += -8;
    if (right) x += 8;
    if (frame >= anims[0]->count()) frame = anims[0]->count() - 1;
    if (frame < 0) frame = 0;
    Animation *a = anims[colour - 1];                // (colour is 1..5 here)
    a->setFrame(frame);                              // 0x403f20
    a->x = x;
    a->y = y;
    a->draw();                                       // v14
}

// ---------------------------------------------------------------- BucketGame

BucketGame::BucketGame(Raytrace::RayTrace *r, Vision::Screen *s) : rt(r), screen(s) {   // 0x43b740
    for (Vision::Sound *&f : fx) f = new Vision::Sound("");   // 0x437290 x4
    ball = new BucketBall(rt);                       // 0x4243c0(0xae8)
    prompt.add("BUCK_VO_RACprompt", false);          // 0x4c46e0
}

BucketGame::~BucketGame() {                          // 0x43b810
    delete ball;                                     // v3
    ball = nullptr;
    for (Vision::Sound *&f : fx) { delete f; f = nullptr; }
}

// 0x43b9c0: the ball drawn on the scene's first overlay; the tree sprite (twice its size) with its stickers: the
// branch, the falling branch (hidden), the swinging bucket, the bucket's hit regions (hidden), the spinning sign
// and the five colour signs (hidden).
void BucketGame::load() {
    ball->load();                                    // v15
    screen->layer(0)->add(ball);                     // 0x41ece0, 0x41e930
    rt->addSprite("bucket_tree", "BUCK_GA_bstree_", 0);   // 0x42d9d0
    Sprite3D *t = rt->findSprite("bucket_tree");
    t->load();                                       // 0x42ef20
    t->scale = 200.0f;                               // +0x84
    t->x = 3960.0f;                                  // +0x78
    t->place();                                      // 0x42f7f0
    t->z = 560.0f;                                   // +0x80
    t->mode = 0;                                     // +0x88
    t->attach("tree_branch", "BUCK_GA_bsbranch_", 100);   // 0x42f390
    Attachment *a = t->attach("falling_branch", "BUCK_GA_brcfall_", 100);
    anim_of(a)->mode = 0;                            // +0x104: once
    a->on = false;
    t->attach("bucket_swing", "BUCK_GA_bucswn_", 90);
    anim_of(rt->findAttached("bucket_swing"))->mode = 2;   // 0x42dd00
    struct { const char *n, *p; int z; } hits[4] = {
        {"bucket_hit", "BUCK_GA_buchit_", 70}, {"bucket_left", "BUCK_GA_buclft_", 80},
        {"bucket_right", "BUCK_GA_bucrht_", 80}, {"bucket_high", "BUCK_GA_buchigh_", 80}};
    for (auto &h : hits) {
        Attachment *b = t->attach(h.n, h.p, h.z);
        anim_of(rt->findAttached(h.n))->mode = 2;
        b->on = false;
    }
    t->attach("sign_spin", "BUCK_GA_signspin_", 80);
    anim_of(rt->findAttached("sign_spin"))->mode = 2;
    static const char *const signs[5] = {"BUCK_GA_signblu_", "BUCK_GA_signgre_", "BUCK_GA_signpur_",
                                         "BUCK_GA_signred_", "BUCK_GA_signyel_"};
    for (int i = 0; i < 5; i++) t->attach(kSigns[i], signs[i], 80)->on = false;
    fx[0]->play();                                   // (not opened yet: nothing)
    sign_colour = 0;
    int r = Vision::vision_random(300);
    spin_until = r + Vision::frame_time() + 300;
    for (bool &d : done) d = false;
    phase = 0;
    static const char *const names[4] = {"buck_fx_signspin", "buck_fx_woodknock", "buck_fx_puztoss",
                                         "buck_fx_branchopn"};   // 0x53645c
    for (int i = 0; i < 4; i++) {
        fx[i]->setName(names[i]);
        fx[i]->open();
    }
    hint_until = 0;
}

void BucketGame::unload() {                          // 0x43bc80
    for (Vision::Sound *f : fx) f->close();
    ball->unload();                                  // v16
}

// 0x43bcb0: the pieces back on the table, the cannon away, the hideout's tune; unless the game was escaped, the
// puzzle game's ending (else the scene's interactive things back on).
void BucketGame::end() {
    for (bool &d : done) d = false;
    Lost_Scene &sc = lost_scene();
    sc.bcursor->colour = 6;                          // 0x5af93c +0xb4
    ball->flying = false;
    table();
    Game::cannon().hide();                           // 0x4e3bf0
    sc.forceNone();                                  // 0x4abf30
    sc.bucket_on = false;                            // 0x5af948
    sc.playMusic(2);                                 // 0x443da0
    phase = 0;
    if (state().last_game != 5) {
        sc.bucket_end.start();                       // 0x4c19e0 on 0x5afc5c
    } else {
        sc.inactivity.start();                       // 0x4c7730 on 0x5b167c
        sc.hotOn(true);                              // 0x442dd0
        sc.signsOn(true);                            // 0x442db0
    }
    sc.tink->wanders = false;                        // +0x13c
    prompt.stop();                                   // v13
}

// 0x43bd70: every 50 ms: the sign, the table; then the phase: 1 the camera flies to the hideout's spot 2 (in
// 9 ms), 2 until there, 3 playing until five hits (the win) or the cannon's time is up (lost), 4 the end.
void BucketGame::update() {
    uint32_t t = Vision::frame_time();
    if (t < 0x32) return;                            // (the clock itself against 50, not the time since +0x24)
    last = t;
    sign();
    table();
    Lost_Scene &sc = lost_scene();
    State &st = state();
    switch (phase) {
    case 1: {
        sc.forceOff();                               // 0x4abf50
        float sp[4];
        st.cameraSpot(3, 2, sp);                     // 0x4ec180
        mover.start(&sc.rt, sp[0], sp[1], sp[2], sp[3], 9);   // 0x431690
        phase = 2;
        break;
    }
    case 2:
        if (!mover.step()) phase = 3;                // 0x431890
        break;
    case 3:
        if (sc.hits >= 5) {
            win();
        } else if (Game::cannon().visible && !Game::cannon().running()) {   // state +0x83c, 0x4e3b80
            phase = 4;
            sc.bucket_bits |= 2;
            st.last_result = 0;
        }
        break;
    case 4:
        end();
        break;
    default:
        break;
    }
}

// 0x43be90: the branch breaks and falls with the map segment; by the game's "Info": the first win gives the
// star (result 2), the second the cap (3), later ones result 4.
void BucketGame::win() {
    phase = 4;
    Sprite3D *t = rt->findSprite("bucket_tree");
    t->hideAttached("tree_branch");
    Attachment *f = rt->findAttached("falling_branch");
    f->on = true;
    anim_of(f)->setFrame(0);
    fx[3]->play();                                   // branchopn
    t->hideAttached("bucket_swing");
    Lost_Scene &sc = lost_scene();
    State &st = state();
    switch (st.saves.gameInfo(kGame)) {              // 0x4e9d70
    case 0:
        sc.bucket_bits |= 4;
        st.last_result = 2;
        sc.buck.star->on = true;                     // 0x5aee00 (+0x2908)
        break;
    case 1:
        sc.bucket_bits |= 8;
        st.last_result = 3;
        sc.buck.cap->on = true;                      // 0x5aedfc (+0x2904)
        break;
    case 2:
        st.last_result = 4;
        break;
    default:
        break;
    }
}

// 0x43bf80: while the game is on, the pieces on the table but those hit, the one carried and the one flying.
void BucketGame::table() {
    Lost_Scene &sc = lost_scene();
    if (!sc.bucket_on) return;
    Sprite3D *t = rt->findSprite("bucket_table");
    if (!t) return;
    for (const char *n : kOnTable) t->showAttached(n);
    for (int i = 0; i < 5; i++)
        if (done[i]) t->hideAttached(kOnTable[i]);
    int c = sc.bcursor->colour;
    if (c >= 1 && c <= 5) t->hideAttached(kOnTable[c - 1]);
    int f = ball->flying ? ball->colour : 6;
    if (f >= 1 && f <= 5) t->hideAttached(kOnTable[f - 1]);
}

// 0x43c120: while the game is on: spinning (0) until its time, then (fewer than five hits) a colour for a while
// (5 s at random plus the skill's delay): drawn from 0..9 tenths, a colour already hit drawn again, 5..9 keep
// the last one drawn (so a colour hit may show). Showing, past its time, fewer than four hits and no hint
// showing: spinning again.
void BucketGame::sign() {
    Lost_Scene &sc = lost_scene();
    if (!sc.bucket_on) return;
    Sprite3D *t = rt->findSprite("bucket_tree");
    t->hideAttached("sign_spin");
    for (const char *n : kSigns) t->hideAttached(n);
    if (sign_colour == 0) {
        uint32_t now = Vision::frame_time();
        if (now >= spin_until && sc.hits < 5) {
            show_until = Vision::vision_random(5000) + delay + now;
            for (;;) {
                int r = Vision::vision_random(1000) / 100;
                if (r > 4) break;
                sign_colour = r + 1;
                if (!done[r]) break;
            }
        }
    } else if (Vision::frame_time() > show_until && sc.hits < 4 && Vision::frame_time() > hint_until) {
        fx[0]->play();
        sign_colour = 0;
        int r = Vision::vision_random(300);
        spin_until = r + Vision::frame_time() + 300;
    }
    if (sign_colour == 0) t->showAttached("sign_spin");
    else if (sign_colour >= 1 && sign_colour <= 5) t->showAttached(kSigns[sign_colour - 1]);
}

// 0x43c340: Tink flies 100 off the blue sign (at 30 degrees) and points at it; the sign keeps its colour 6 s.
void BucketGame::hint() {
    Lost_Scene &sc = lost_scene();
    Tinkerbell3D *tk = sc.tink;
    tk->clearQueue();                                // 0x4ddf30
    float p[3];
    sc.rt.stickerPos("sign_blue", p);                // 0x42df30
    float ox, oz;
    Tinkerbell3D::polar(p[0], p[2], &ox, &oz, 30.0f, 100.0f);   // 0x4da950
    tk->pushBasic(2, ox, p[1], oz);                  // 0x4de020
    tk->point(p[0], p[1], p[2]);                     // 0x4db7b0
    hint_until = Vision::frame_time() + 6000;
    sc.tinkHome();                                   // 0x4abb10
}

// 0x43c420: the table (made once) with the five pieces on it, each shown while a piece is hidden in the area
// (the exe asks "any hidden" for each).
void BucketGame::setupTable() {
    if (!rt->findSprite("bucket_table")) {
        rt->addSprite("bucket_table", "BUCK_GA_bstable_", 1);   // port: the exe pushes two arguments, the third is the
                                                          // caller's saved edi (non-zero): a hit mask
        Sprite3D *t = rt->findSprite("bucket_table");
        t->load();
        t->scale = 200.0f;
        t->x = 3970.0f;
        t->place();
        t->z = 780.0f;
        static const char *const p[5] = {"BUCK_GA_puzblu_", "BUCK_GA_puzgre_", "BUCK_GA_puzpur_", "BUCK_GA_puzred_",
                                         "BUCK_GA_puzyel_"};
        for (int i = 0; i < 5; i++) t->attach(kOnTable[i], p[i], 100)->period = 10000000;   // +0x184
    }
    for (const char *n : kOnTable)
        rt->findAttached(n)->on = state().anyLost(5);   // 0x4eef20
}

// 0x43c5b0: the game begins: the pieces all "in the game" (5), the stickers reset, the skill's delay; the
// cannon for a game won before (180 / 120 / 90 s).
void BucketGame::start() {
    fx[0]->play();
    sign_colour = 0;
    int r = Vision::vision_random(300);
    spin_until = r + Vision::frame_time() + 300;
    Lost_Scene &sc = lost_scene();
    State &st = state();
    sc.bucket_on = true;
    sc.playMusic(1);
    for (TokenInfo &t : st.lost) t.state = 5;        // +0x172c.. (0x18 apart)
    sc.tokens->visible = false;                      // 0x5b0138 +0x7c
    sc.refreshTokens();                              // 0x442e00
    setupTable();
    for (bool &d : done) d = false;
    rt->findSprite("bucket_tree");
    rt->findAttached("tree_branch")->on = true;
    rt->findAttached("bucket_swing")->on = true;
    rt->findAttached("falling_branch")->on = false;
    sc.hits = 0;
    int skill = 0;                                   // port: the exe leaves it uninitialised when the key is missing
    st.saves.getSkill(&skill);                       // 0x4e97a0
    switch (skill) {
    case 1: delay = 9000; break;
    case 2: delay = 3000; break;
    case 3: delay = 1000; break;
    default: break;
    }
    int info = st.saves.gameInfo(kGame);
    if (info == 1 || info == 2) {
        int secs;
        switch (skill) {
        case 1: secs = 180; break;
        case 2: secs = 120; break;
        case 3: secs = 90; break;
        default: secs = skill; break;
        }
        Game::cannon().visible = true;               // state +0x83c
        Game::cannon().start(secs);                  // 0x4e3a50
    }
    sc.signsOn(false);                               // 0x442db0
    sc.hotOn(false);                                 // 0x442dd0
    phase = 1;
    st.last_game = 4;
    prompt.start();                                  // v12
}

// 0x43c780: the five pieces flat; the one under the mouse raised.
bool BucketGame::hover(Attachment *a) {
    rt->findSprite("bucket_table");
    for (const char *n : kOnTable) anim_of(rt->findAttached(n))->setFrame(0);
    if (!a) return false;
    Attachment *hit = nullptr;
    for (const char *n : kOnTable)
        if (a->name == n) hit = a;
    if (!hit) return false;
    anim_of(hit)->setFrame(1);
    return true;
}

// 0x43c890: a piece clicked on the table goes onto the cursor.
bool BucketGame::pick(Attachment *a) {
    if (!a) return false;
    int c = 6;
    for (int i = 0; i < 5; i++)
        if (a->name == kOnTable[i]) c = i + 1;
    if (c == 6) return false;
    fx[2]->play();                                   // +0x374: puztoss
    rt->findSprite("bucket_table")->hideAttached(a->name.c_str());
    lost_scene().bcursor->colour = c;
    return true;
}

void BucketGame::toss(float px, float py, int colour) {   // 0x43c960
    if (colour == 6) return;
    ball->toss(px, py, rt->camz, colour);            // 0x42b540
    fx[1]->play();                                   // +0x6bc: woodknock
}

}  // namespace Game
