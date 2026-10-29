// Peter and Wendy in London, reconstructed from "Tinker Bell.exe" (see london_chars.h).
#include "london_chars.h"
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include "tb.h"

namespace Game {

using Vision::AnimationHelper;
using Raytrace::Attachment;
using Raytrace::Sprite3D;

// ---------------------------------------------------------------- helpers
static const float kTop = 800.0f;                    // 0x516d70: Peter's height before he lands

// new AnimationHelper(prefix, mode) (0x4243c0 + 0x4e3040), 12 fps (+0x108), hit mode 1 (v6 (1, 1)).
static AnimationHelper *helper(const char *p, int mode, bool hit = true) {
    AnimationHelper *h = new AnimationHelper(p, mode);
    h->fps = 12;
    if (hit) h->setHit(1, true);
    return h;
}

// The sprite's height above where 0x42f7f0 would stand it (the pattern 0x42f700 x2, 0x42f7f0, 0x42f700, 0x42f740
// that the Fly and Land code inlines).
static float above_ground(Sprite3D *s) {
    float py = s->y;
    float qx = s->x, qy = s->y, qz = s->z;
    s->place();
    float ry = s->y;
    s->setPos(qx, qy, qz);
    return (float)((double)py - ry);
}

// Stands the sprite at p, on the ground, then raised by p.y.
static void stand_at(Sprite3D *s, const float p[3]) {
    s->setPos(p[0], p[1], p[2]);
    s->place();
    s->y = (float)((double)p[1] + s->y);
}

// The tail of 0x4bb8f0 / 0x4bd720: from where the sprite is down to 0.1 above the ground, half way at t = 0.5.
static void curve_down(Curve3 &c, Sprite3D *s) {
    float px = s->x, pz = s->z;
    float dy = above_ground(s);
    float A[3] = {px, dy, pz}, B[3] = {px, 0.1f, pz};   // 0x3dcccccd
    float d[3], h[3], M[3];
    for (int i = 0; i < 3; i++) d[i] = (float)((double)B[i] - A[i]);   // 0x425eb0
    for (int i = 0; i < 3; i++) h[i] = (float)((double)d[i] / 2.0f);   // 0x425f10
    for (int i = 0; i < 3; i++) M[i] = (float)((double)A[i] + h[i]);   // 0x425e50
    c.set0(A);
    c.set1(M);
    c.set2(B);
}

// 0x42b520: the camera height in world units at the sprites' scale 100.
static double eye_world(Raytrace::RayTrace *rt) {
    return ((double)rt->eyef + 160.0f) * 590.0f * 100.0f / 24000.0f;   // 0x513d1c 0x513d24 0x513da0 0x513d18
}

static Attachment *find_or_attach(Sprite3D *s, const char *n) {   // 0x42f350, else 0x42f390 + 0x42e4a0
    Attachment *a = s->findAttached(n);
    if (!a) a = s->attach(n, n, 0);
    a->on = false;
    return a;
}
static Attachment *attach_off(Sprite3D *s, const char *n, const char *p) {   // 0x42f390 + 0x42e4a0
    Attachment *a = s->attach(n, p, 0);
    if (a) a->on = false;                            // port: the exe would crash on a name taken already
    return a;
}
static void mouth_frame(Attachment *a, int letter) { // 0x4bd510: the mouth on, showing the letter's frame
    if (!a) return;
    a->on = true;
    static_cast<Vision::Animation *>(a->g)->setFrame(letter);   // 0x403f20 on +0x150
}

// The blink both talking states run (0x4bb480 / 0x4bd330 / 0x4bb810 / 0x4bd610): eyes shut for 83 ms every
// 2..5 seconds.
static void blink(Attachment *eyes, uint32_t &t) {
    if (t < Vision::frame_time() && !eyes->on) {
        t = Vision::frame_time() + 0x53;
        eyes->on = true;
    }
    if (t < Vision::frame_time() && eyes->on) {
        eyes->on = false;
        t = (uint32_t)Vision::vision_random(3000) + 2000 + Vision::frame_time();
    }
}

// ---------------------------------------------------------------- LondonChar
LondonChar::~LondonChar() {
    for (State *s : owned) delete s;                 // port: the exe frees the states without their helpers
    for (Vision::Ras *r : packs) if (r) Vision::ras_close(r);   // 0x419620
    packs.clear();
}
bool LondonChar::ready() { return cur && cur->ready(); }          // port: the exe has a state by then
void LondonChar::mouth(int l) { if (cur) cur->mouth(l); }
void LondonChar::rest() { if (cur) cur->rest(this); }
bool LondonChar::isTalk(const State *s) const {
    return s == talk[0] || s == talk[1] || s == talk[2] || s == talk[3];
}
void LondonChar::request(State *s) {
    if (s != idle || !isTalk(next)) next = s;
}
void LondonChar::openPacks(const char *a, const char *b) {   // 0x419b90, 0x41aa20 load, 0x41a920 current
    for (const char *n : {a, b}) {
        Vision::Ras *r = Vision::ras_open(n, "", &Vision::data_device());
        r->load();
        Vision::ras_set_current(r);
        packs.push_back(r);
    }
}

namespace {

using State = LondonChar::State;

// PeterStateAnimation (0x516c78) / WendyStateAnimation (0x516d90): plays the helper; when it ends asks for
// *after (Bow / TurnToPlayer: idle (0x4bb580); TurnToWendy: FacingWendy (0x4bb5c0); ShowHand: IdleHand (0x4bb600);
// Wendy's ShowClock / TransitionForward: Forward (0x4bd450), HideClock / TurnToPlayer: idle (0x4bd6e0),
// TransitionBack: Back (0x4bd410), TransitionPoint: Point (0x4bd4b0), TransitionPointDown: RootIdle (0x4bd540),
// TurnToPeter: FacingPeter (0x4bd580)).
struct AnimState : State {
    AnimState(const char *p, State **after, bool rev = false) : h(helper(p, 0)), after(after) {
        if (rev) h->reverse();                       // 0x4e3220
    }
    ~AnimState() override { delete h; }
    void enter(LondonChar *c) override { c->sprite->anim = h; h->start(); }   // 0x4bb660 / 0x4bccb0
    bool leave(LondonChar *) override { return !h->playing(); }               // 0x4bb570
    void tick(LondonChar *c) override { if (!h->playing()) c->request(*after); }
    AnimationHelper *h;
    State **after;
};

// PeterStateIdle (0x516c40, 0x4baf10) / WendyStateIdle (0x516dc8, 0x4bcd10): the root animation for 5 seconds at
// a time, then one of the others at random.
struct IdleState : State {
    IdleState(bool wendy, const char *root, const char *const *idles, const int *fps, int n) : wendy(wendy) {
        hs.push_back(helper(root, 0));
        for (int i = 0; i < n; i++) {
            AnimationHelper *h = new AnimationHelper(idles[i], 0);
            h->fps = fps[i];
            h->setHit(1, true);
            hs.push_back(h);
        }
        cur = hs[0];
    }
    ~IdleState() override { for (AnimationHelper *h : hs) delete h; }
    void enter(LondonChar *) override { t0 = Vision::frame_time(); }   // 0x4bb220
    bool leave(LondonChar *) override {              // 0x4bb230 / 0x4bd080
        if (cur == hs[0]) {
            cur->stop();                             // 0x403c00
            return !cur->playing();
        }
        if (!(wendy && cur == hs[2]) && cur->dir == 1 && cur->current() + 2 < cur->count())
            cur->reverse();                          // back to its first frame
        return !cur->playing();
    }
    void tick(LondonChar *c) override {              // 0x4bb140 / 0x4bcfa0
        AnimationHelper *h = dynamic_cast<AnimationHelper *>(c->sprite->anim);   // 0x4f2f63
        // port: Wendy's copy calls 0x4e3110 even without one; never the case in the game
        if (h && h->playing()) return;
        if (Vision::frame_time() - t0 <= 5000) cur = hs[0];
        else {
            t0 = Vision::frame_time();
            int r = Vision::vision_random((int)hs.size() - 1);   // 0x425d40
            cur = hs[(size_t)r + 1];
        }
        c->sprite->anim = cur;
        cur->forward();                              // 0x4e31b0
        cur->start();
    }
    bool wendy;
    std::vector<AnimationHelper *> hs;               // +0x08
    AnimationHelper *cur;                            // +0x14
    uint32_t t0 = 0;                                 // +0x18
};

// PeterStateTalkIdle (0x516c5c, 0x4bb2b0) / WendyStateTalk (0x516de4, 0x4bd120): a talking animation with the
// mouth and blinking eyes. Wendy's reverses half way through "LOND_VO_WENsign1".
struct TalkState : State {
    TalkState(LondonChar *c, const char *p, const char *mouth, const char *eyes, bool wendy)
        : h(helper(p, 0)), wendy(wendy) {
        m = find_or_attach(c->sprite, mouth);
        e = find_or_attach(c->sprite, eyes);
    }
    ~TalkState() override { delete h; }
    void enter(LondonChar *c) override {             // 0x4bb3c0 / 0x4bd230
        t = (uint32_t)Vision::vision_random(3000) + 2000 + Vision::frame_time();
        c->sprite->anim = h;
        h->forward();
        h->start();
        if (!wendy) return;
        Vision::VoiceOver &vo = Vision::VoiceOver::get();
        if (vo.isPlaying("LOND_VO_WENsign1")) {
            vo_start = Vision::frame_time();
            vo_half = vo.lengthMs() >> 1;
        } else {
            vo_half = 0xffffffffu;
            vo_start = 0xffffffffu;
        }
    }
    bool leave(LondonChar *) override {              // 0x4bb410 / 0x4bd2c0
        e->on = false;
        m->on = false;
        if (wendy) { vo_half = 0; vo_start = 0; }
        if (h->dir == 1 || h->current() + 1 == h->count()) {
            h->reverse();
            if (!h->playing()) h->start();
        }
        return !h->playing();
    }
    void tick(LondonChar *) override {               // 0x4bb480 / 0x4bd330
        blink(e, t);
        if (wendy && vo_half < Vision::frame_time() - vo_start) {
            h->reverse();
            h->start();
            vo_half = 0xffffffffu;
            vo_start = 0xffffffffu;
        }
    }
    bool ready() override { return true; }           // 0x4bb500 / 0x4bee70
    void mouth(int l) override { mouth_frame(m, l); }   // 0x4bd510
    void rest(LondonChar *c) override {              // 0x4bb510 / 0x4bd3d0
        if (!m) return;
        m->on = false;
        c->request(c->idle);
    }
    AnimationHelper *h;                              // +0x04
    Attachment *m, *e;                               // +0x08 +0x0c
    uint32_t t = 0;                                  // +0x10: the next blink
    uint32_t vo_start = 0, vo_half = 0;              // Wendy +0x14 +0x1c
    bool wendy;
};

// PeterStateFacingWendy (0x516d20, 0x4bb680): looking at Wendy, the mouth at rest between lines.
struct PeterFacingWendy : State {
    explicit PeterFacingWendy(LondonChar *c) : h(helper("LOND_CA_PETlwidle_", 2)) {
        m = attach_off(c->sprite, "PER_mout", "PER_mout");
        mr = attach_off(c->sprite, "PER_moutrest", "PER_moutrest");
        e = attach_off(c->sprite, "PER_eyes", "PER_eyes");
    }
    ~PeterFacingWendy() override { delete h; }
    void enter(LondonChar *c) override {             // 0x4bb790
        t = (uint32_t)Vision::vision_random(3000) + 2000 + Vision::frame_time();
        c->sprite->anim = h;
        h->start();
        m->on = false;
        mr->on = true;
    }
    bool leave(LondonChar *) override {              // 0x4bb7e0
        e->on = false;
        mr->on = false;
        h->stop();
        return true;
    }
    void tick(LondonChar *) override { blink(e, t); }   // 0x4bb810
    bool ready() override { return true; }
    void mouth(int l) override {                     // 0x4bb890
        if (!m) return;
        mr->on = false;
        mouth_frame(m, l);
    }
    void rest(LondonChar *) override {               // 0x4bb8d0
        if (!m) return;
        mr->on = true;
        m->on = false;
    }
    AnimationHelper *h;
    Attachment *m, *mr, *e;                          // +0x08 +0x0c +0x10
    uint32_t t = 0;                                  // +0x14
};

// WendyStateFacingPeter (0x516efc): the mouth and eyes over whatever she shows.
struct WendyFacingPeter : State {
    explicit WendyFacingPeter(LondonChar *c) {
        m = attach_off(c->sprite, "WEL_mout", "WEL_mout");
        e = attach_off(c->sprite, "WEL_eyes", "WEL_eyes");
    }
    void enter(LondonChar *) override {              // 0x4bd5c0
        t = (uint32_t)Vision::vision_random(3000) + 2000 + Vision::frame_time();
    }
    bool leave(LondonChar *) override { m->on = false; e->on = false; return true; }   // 0x4bd5f0
    void tick(LondonChar *) override { blink(e, t); }   // 0x4bd610
    bool ready() override { return true; }
    void mouth(int l) override { mouth_frame(m, l); }   // 0x4bd690
    void rest(LondonChar *) override { if (m) m->on = false; }   // 0x4bd6c0
    Attachment *m, *e;                               // +0x04 +0x08
    uint32_t t = 0;                                  // +0x0c
};

// A held animation with a mouth: Peter's IdleHand (0x516d04), Wendy's Point (0x516ea8) and Forward (0x516e70).
struct MouthState : State {
    enum Kind { IdleHand, Point, Forward };
    MouthState(LondonChar *c, Kind k, const char *p, int mode, const char *mn, const char *mp)
        : h(helper(p, mode)), kind(k) {
        m = attach_off(c->sprite, mn, mp);
    }
    ~MouthState() override { delete h; }
    void enter(LondonChar *c) override {
        if (kind == Forward) {                       // 0x4bd490: its first frame, held
            h->setFrame(0);
            c->sprite->anim = h;
            return;
        }
        c->sprite->anim = h;                         // 0x4bb660 / 0x4bccb0
        h->start();
    }
    bool leave(LondonChar *) override {
        if (kind == Forward) return true;            // 0x4bb500
        if (m) m->on = false;                        // 0x4bd4f0
        h->stop();
        return true;
    }
    void tick(LondonChar *) override {
        if (kind == Point && !h->playing()) h->start();   // 0x4bccf0 (the others: 0x402b80, nothing)
    }
    bool ready() override { return true; }
    void mouth(int l) override { mouth_frame(m, l); }   // 0x4bd510
    void rest(LondonChar *) override { if (m) m->on = false; }   // 0x4bb640
    AnimationHelper *h;
    Attachment *m;                                   // +0x08
    Kind kind;
};

// WendyStateRootIdle (0x516dac): the root animation, over and over.
struct RootIdle : State {
    RootIdle() : h(helper("GLOB_CA_WENroot_", 0)) {}
    ~RootIdle() override { delete h; }
    void enter(LondonChar *c) override { c->sprite->anim = h; }   // 0x4bccd0
    bool leave(LondonChar *) override { h->stop(); return true; }   // 0x4bcce0
    void tick(LondonChar *) override { if (!h->playing()) h->start(); }   // 0x4bccf0
    AnimationHelper *h;
};

// WendyStateBack (0x516f6c) / WendyStateFlyIdle (0x516f50): nothing to do.
struct EmptyState : State {};

// PeterStateLand (0x516d3c, 0x4bb8f0): two seconds down the curve from where he waits above the street.
struct PeterLand : State {
    explicit PeterLand(LondonChar *c) : h(helper("GLOB_CA_PETflyid_", 2, false)) { curve_down(curve, c->sprite); }
    ~PeterLand() override { delete h; }
    void enter(LondonChar *c) override {             // 0x4bbb50
        t0 = Vision::frame_time();
        c->sprite->anim = h;
        h->visible = true;                           // +0x7c
        h->start();
    }
    bool leave(LondonChar *c) override {             // 0x4bbb80
        if ((float)((double)(int64_t)(Vision::frame_time() - t0) * 0.0005f) < 1.0f) return false;   // 0x514504
        float p[3];
        curve.at(1.0f, p);
        stand_at(c->sprite, p);
        return true;
    }
    void tick(LondonChar *c) override {              // 0x4bbc60
        float t = (float)((double)(int64_t)(Vision::frame_time() - t0) * 0.0005f);
        if (!(t < 1.0f)) { c->request(c->idle); return; }
        float p[3];
        curve.at(t, p);
        stand_at(c->sprite, p);
    }
    AnimationHelper *h;
    Curve3 curve;                                    // +0x08
    uint32_t t0 = 0;                                 // +0x0c
};

// PeterStateFly (0x516d58): he rises with the camera; at 800 above the street he is done.
struct PeterFly : State {
    PeterFly() : h(helper("LOND_CA_PEThndfly_", 2, false)) {}
    ~PeterFly() override { delete h; }
    void enter(LondonChar *c) override {             // 0x4bbd70
        float off = above_ground(c->sprite);
        c->sprite->anim = h;
        h->start();
        f8 = (float)(eye_world(c->rt) / ((double)c->sprite->scale * 0.01f) - off);   // 0x513d4c
    }
    bool leave(LondonChar *) override { h->stop(); return true; }   // 0x4bcce0
    void tick(LondonChar *c) override {              // 0x4bbe50
        Sprite3D *s = c->sprite;
        float px = s->x, pz = s->z;
        above_ground(s);
        float ny = (float)(eye_world(c->rt) / ((double)s->scale * 0.01f) - f8);
        float p[3] = {px, ny, pz};
        stand_at(s, p);
        if (!(ny < kTop)) c->request(c->idle);
    }
    AnimationHelper *h;
    float f8 = 0;                                    // +0x08: the camera height minus his, at the start
};

// WendyStateFly (0x516f34, 0x4bd720): her flying loop down the (flat) curve for two seconds, then she is gone
// and asks for FlyIdle.
struct WendyFly : State {
    WendyFly(LondonChar *c, State **after) : h(helper("GLOB_CA_WENflyid_", 2, false)), after(after) {
        curve_down(curve, c->sprite);
    }
    ~WendyFly() override { delete h; }
    void enter(LondonChar *) override {}             // 0x4baf00
    bool leave(LondonChar *) override { return true; }   // 0x4bb500
    void tick(LondonChar *c) override {              // 0x4bd980
        if (!h->playing()) h->start();
        if (!started) {
            t0 = Vision::frame_time();
            c->sprite->anim = h;
            h->start();
            started = true;
            return;
        }
        float t = (float)((double)(int64_t)(Vision::frame_time() - t0) * 0.0005f);
        if (!(t < 1.0f)) {
            c->sprite->anim->visible = false;
            started = false;
            c->request(*after);
            return;
        }
        float p[3];
        curve.at(t, p);
        stand_at(c->sprite, p);
    }
    AnimationHelper *h;
    Curve3 curve;                                    // +0x08
    bool started = false;                            // +0x0c
    uint32_t t0 = 0;                                 // +0x10
    State **after;                                   // FlyIdle (+0x50)
};

}  // namespace

// ---------------------------------------------------------------- PeterInLondon
PeterInLondon::PeterInLondon(Raytrace::RayTrace *r) {   // 0x4bbf90
    rt = r;
    openPacks("Peter_char", "LOND_PET");
    sprite = rt->addSprite("GLOB_CA_PETroot_", "GLOB_CA_PETroot_", 1);   // 0x42d9d0
    sprite->scale = 130.0f;
    sprite->load();
    sprite->mode = 3;
    sprite->setPos(2843.4f, 800.0f, 2821.4f);        // 0x4531b666, 0x44480000, 0x45305666
    sprite->place();
    sprite->y = sprite->y + kTop;
    sprite->anim->visible = false;
    static const char *const idles[] = {"GLOB_CA_PETidle1_", "GLOB_CA_PETidle2_"};
    static const int fps[] = {12, 12};
    idle = own(new IdleState(false, "GLOB_CA_PETroot_", idles, fps, 2));
    static const char *const talks[] = {"GLOB_CA_PETroottk_", "GLOB_CA_PETtalk1_", "GLOB_CA_PETtalk2_",
                                        "GLOB_CA_PETtalk3_"};
    for (int i = 0; i < 4; i++) talk[i] = own(new TalkState(this, talks[i], "PET_mout", "PET_eyes", false));
    st_bow = own(new AnimState("LOND_CA_PETbows_", &idle));
    st_turnwendy = own(new AnimState("LOND_CA_PETlokwen_", &st_facing));
    st_facing = own(new PeterFacingWendy(this));
    st_turnplayer = own(new AnimState("LOND_CA_PETlokwen_", &idle, true));
    st_land = own(new PeterLand(this));
    st_showhand = own(new AnimState("LOND_CA_PEThand_", &st_idlehand));
    st_idlehand = own(new MouthState(this, MouthState::IdleHand, "LOND_CA_PEThandidl_", 2, "PET flying mouth",
                                     "PET_mout"));
    st_fly = own(new PeterFly());
    request(idle);
}

PeterInLondon::~PeterInLondon() {                    // 0x4bc560
    if (Sprite3D *s = rt->findSprite("GLOB_CA_PETroot_")) s->anim = nullptr;   // 0x42dc70
    // port: the states (and their helpers) go before the packs, see ~LondonChar
    for (State *s : owned) delete s;
    owned.clear();
}

void PeterInLondon::update() {                       // 0x4bc880
    if (cur == next) { if (cur) cur->tick(this); return; }
    if (cur && !cur->leave(this)) return;
    cur = next;
    next->enter(this);
}

static bool any_of(const char *line, const char *const *names) {   // 0x423840 against each
    for (; *names; names++) if (strcasecmp(line, *names) == 0) return true;
    return false;
}

void PeterInLondon::say(const char *line) {          // 0x4bc8b0
    if (getenv("TB_DEBUG_LONDON")) tb_log("london %u: Peter %s", Vision::frame_time(), line);
    static const char *const a[] = {"LOND_VO_PETsayhi", "LOND_VO_PETtnkwen", "LOND_VO_PETexplan",
                                    "LOND_VO_PETtnkhlp", "LOND_VO_PETupdown", "LOND_VO_PETentrdw",
                                    "LOND_VO_PEThand", "LOND_VO_PETcrow", "LOND_VO_PETstar", nullptr};
    static const char *const b[] = {"LOND_VO_PETgoidle", "LOND_VO_PETflyidl", "LOND_VO_PETentrid",
                                    "LOND_VO_PETentrdi", nullptr};
    static const char *const d[] = {"LOND_VO_PETletsgo", "LOND_VO_PETfly1", "LOND_VO_PETtink",
                                    "LOND_VO_PETentrup", nullptr};
    static const char *const w[] = {"LOND_VO_PETgreet", "LOND_VO_PETgame", "LOND_VO_PETdust", nullptr};
    if (any_of(line, a)) request(talk[0]);
    if (any_of(line, b)) request(talk[1]);
    else if (strcasecmp(line, "LOND_VO_PETangry") == 0) request(talk[2]);
    else if (any_of(line, d)) request(talk[3]);
    if (any_of(line, w) && cur != st_facing) request(st_turnwendy);
    Vision::VoiceOver &vo = Vision::VoiceOver::get();
    vo.registerLine(line, this);                     // 0x4c91b0
    vo.play(line);                                   // 0x4c9400
}

// ---------------------------------------------------------------- WendyInLondon
WendyInLondon::WendyInLondon(Raytrace::RayTrace *r) {   // 0x4bdaf0
    rt = r;
    openPacks("Wendy_char", "LOND_WEN");
    sprite = rt->addSprite("GLOB_CA_WENroot_", "GLOB_CA_WENroot_", 1);
    sprite->scale = 130.0f;
    sprite->load();
    sprite->mode = 3;
    sprite->setPos(2690.1f, 0.1f, 2690.1f);          // 0x4528219a, 0x3dcccccd
    sprite->place();
    sprite->y = sprite->y + 0.1f;                    // 0x516f84
    sprite->anim->visible = true;
    static const char *const idles[] = {"GLOB_CA_WENidle1_", "GLOB_CA_WENidle2_", "GLOB_CA_WENidle3_"};
    static const int fps[] = {12, 10, 12};
    idle = own(new IdleState(true, "GLOB_CA_WENroot_", idles, fps, 3));
    static const char *const talks[] = {"GLOB_CA_WENroottk_", "GLOB_CA_WENtalk1_", "GLOB_CA_WENtalk2_",
                                        "GLOB_CA_WENtalk3_"};
    for (int i = 0; i < 4; i++) talk[i] = own(new TalkState(this, talks[i], "WEN_mout", "WEN_eyes", true));
    st_showclock = own(new AnimState("TICK_CA_WENclock_", &st_forward));
    st_hideclock = own(new AnimState("TICK_CA_WENclock_", &idle, true));
    st_toback = own(new AnimState("TICK_CA_WENback_", &st_back));
    st_back = own(new EmptyState());
    st_toforward = own(new AnimState("TICK_CA_WENback_", &st_forward, true));
    st_forward = own(new MouthState(this, MouthState::Forward, "TICK_CA_WENback_", 0, "clock lips", "WEN_mout"));
    st_topoint = own(new AnimState("LOND_CA_WENpoint_", &st_point));
    st_point = own(new MouthState(this, MouthState::Point, "LOND_CA_WENpntidl_", 0, "WEP_mout", "WEP_mout"));
    st_pointdown = own(new AnimState("LOND_CA_WENseeland_", &st_rootidle));
    st_turnpeter = own(new AnimState("LOND_CA_WENlokpet_", &st_facing));
    st_facing = own(new WendyFacingPeter(this));
    st_turnplayer = own(new AnimState("LOND_CA_WENlokply_", &idle));   // 0x4bcc00
    st_fly = own(new WendyFly(this, &st_flyidle));
    st_flyidle = own(new EmptyState());
    st_rootidle = own(new RootIdle());
    request(st_rootidle);
}

WendyInLondon::~WendyInLondon() {                    // 0x4be390
    if (Sprite3D *s = rt->findSprite("GLOB_CA_WENroot_")) s->anim = nullptr;
    for (State *s : owned) delete s;
    owned.clear();
}

void WendyInLondon::update() {                       // 0x4be780
    if (!next || cur == next) { if (cur) cur->tick(this); return; }
    if (cur && !cur->leave(this)) return;
    State *n = next;
    next = nullptr;
    cur = n;
    n->enter(this);
}

void WendyInLondon::position(float out[3]) {         // 0x4be7d0
    float px = sprite->x, pz = sprite->z;
    float dy = above_ground(sprite);
    out[0] = px;
    out[1] = dy;
    out[2] = pz;
}

void WendyInLondon::say(const char *line) {          // 0x4be880
    if (getenv("TB_DEBUG_LONDON")) tb_log("london %u: Wendy %s", Vision::frame_time(), line);
    static const char *const g0[] = {"LOND_VO_WENaround", "LOND_VO_WENokcur", nullptr};
    static const char *const g1[] = {"LOND_VO_WENleft", "LOND_VO_WENright", "LOND_VO_WENup", "LOND_VO_WENdown",
                                     "LOND_VO_WENhelp1", "LOND_VO_WENinter1", "LOND_VO_WENribbn1",
                                     "LOND_VO_WENcursr1", "LOND_VO_WENthank1", "TICK_VO_WENwin1", "LOND_VO_WENyes",
                                     "LOND_VO_WENletsgo", "TICK_VO_WENclock", nullptr};
    static const char *const g2[] = {"LOND_VO_WENleftxp", "LOND_VO_WENrighxp", "LOND_VO_WENupxp",
                                     "LOND_VO_WENdownxp", "LOND_VO_WENmouse1", "LOND_VO_WENsign2",
                                     "LOND_VO_WENsign3", "TICK_VO_WENgreet1", "TICK_VO_WENface1",
                                     "TICK_VO_WENgscap", "LOND_VO_WENsign1", nullptr};
    static const char *const g3[] = {"LOND_VO_WENleftid", "LOND_VO_WENrghtid", "LOND_VO_WENupid",
                                     "LOND_VO_WENdownid", "LOND_VO_WENfar", "LOND_VO_WENcloser", "LOND_VO_WENhelp2",
                                     "LOND_VO_WENmseidl", "LOND_VO_WENinter2", "LOND_VO_WENribbn2",
                                     "LOND_VO_WENcursr2", "LOND_VO_WENbadcur", "LOND_VO_WENonme",
                                     "LOND_VO_WENonme2", "LOND_VO_WENidlsn2", "LOND_VO_WENidlsn3",
                                     "GLOB_VO_WENidle1", "GLOB_VO_WENidle2", "GLOB_VO_WENidle3", "TICK_VO_WENtree1",
                                     "TICK_VO_WENprompt", "TICK_VO_WENidltrn", "LOND_VO_WENgoidle", nullptr};
    if (any_of(line, g0)) request(talk[0]);
    else if (any_of(line, g1)) request(talk[1]);
    else if (any_of(line, g2)) request(talk[2]);
    else if (any_of(line, g3) || (strcasecmp(line, "LOND_VO_WENidlsn1") == 0 && cur != st_forward))
        request(talk[3]);
    Vision::VoiceOver &vo = Vision::VoiceOver::get();
    vo.registerLine(line, this);
    vo.play(line);
}

}  // namespace Game
