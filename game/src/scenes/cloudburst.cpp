// Cloud Burst, reconstructed from "Tinker Bell.exe" (see cloudburst.h).
#include "cloudburst.h"
#include "starry.h"
#include "state.h"
#include "cannon.h"
#include "tink3d.h"
#include "voiceover.h"
#include "../vision/animation.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tb.h"

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;

static VoiceOver &vo() { return VoiceOver::get(); }  // the state's +0x3e8
static void hide(Sprite3D *s) { if (s && s->anim) s->anim->visible = false; }   // (the null tests the exe makes)
static void show(Sprite3D *s) { s->anim->visible = true; }   // +0x70 +0x7c

static const float kMidZ = 2687.6f;                  // 0x515240: the middle of the stage
static const float kStageX = 6800.1f;                // 0x45d480cd
static const float kStageY = 200.1f;                 // 0x4348199a
static const float kLeftZ = 2260.1f;                 // 0x450d419a
static const float kRightZ = 3115.1f;                // 0x4542b19a

// MSVC 6's random_shuffle (0x472380 for pairs, 0x457be0 for ints, also inlined): each element from the second on
// swapped with one before it (never itself).
template <class T> static void ms_shuffle(std::vector<T> &v) {
    if (v.empty()) return;
    unsigned dd = 1;
    for (size_t i = 1; i < v.size(); i++, dd++) {
        unsigned rm = 0x7fff, rn = (unsigned)crt_rand() & 0x7fff;   // 0x4f54da
        while (rm < dd && rm != ~0u) {
            rm = rm << 15 | 0x7fff;
            rn = rn << 15 | 0x7fff;
        }
        std::swap(v[i], v[rn % dd]);
    }
}

// ---------------------------------------------------------------- the pool
void CloudQuestionPool::add(int qi, int g) {         // 0x471a20
    if (g >= 0 && g < 3) ids[g].push_back(qi);
}

// 0x471be0: by skill (and a game won before): how many of each group; every riddle into its pool (on top of what
// is there: the exe's), each pool shuffled 0..9 times.
void CloudQuestionPool::start() {
    State &st = Game::state();
    int info = st.saves.gameInfo("Cloud Burst Game");   // 0x4e9d70 (*0x543504)
    int skill = 0;
    st.saves.getSkill(&skill);                       // 0x4e97a0
    switch (skill) {
    case 1:
        if (info == 0) { left[0] = 5; left[1] = 0; }
        else { left[0] = 3; left[1] = 3; }
        left[2] = 0;
        break;
    case 2:
        if (info == 0) { left[0] = 3; left[1] = 3; left[2] = 0; }
        else { left[0] = 2; left[1] = 4; left[2] = 2; }
        break;
    case 3:
        if (info == 0) { left[0] = 2; left[1] = 3; left[2] = 2; }
        else { left[0] = 1; left[1] = 4; left[2] = 3; }
        break;
    default: break;
    }
    for (int g = 0; g < 3; g++) {
        for (int qi : ids[g]) pool[g].push_back(std::make_pair(qi, 0));   // 0x472130
        int r = vision_random(10);                   // 0x425d40 (the state's +8)
        for (; r > 0; r--) ms_shuffle(pool[g]);      // 0x472380 (the third inlined)
    }
}

// 0x471e70: the groups still due, shuffled 0..9 times; the first one's riddle asked least (the first such), one
// more time asked.
int CloudQuestionPool::next() {
    std::vector<int> gs;
    for (int g = 0; g < 3; g++)
        if (left[g]) gs.push_back(g);                // 0x4561b0
    int r = vision_random(10);
    for (; r > 0; r--) ms_shuffle(gs);               // 0x457be0
    // port: the exe reads gs[0] with the vector's range check (0x455f10 throws when it is empty)
    if (gs.empty()) return pool[group].empty() || cur < 0 ? 0 : pool[group][cur].first;
    int g = gs[0];
    if (g >= 0 && g < 3) {
        group = g;                                   // +0x70
        std::vector<std::pair<int, int>> &p = pool[g];
        int m = 0;
        for (int i = 1; i < (int)p.size(); i++)
            if (p[i].second < p[m].second) m = i;
        cur = m;                                     // +0x6c
    }
    // port: the exe takes the end of an empty pool here (never so: a group is due only while it has riddles)
    if (pool[group].empty()) return 0;
    pool[group][cur].second++;
    return pool[group][cur].first;
}

void CloudQuestionPool::removeCurrent() {            // 0x472060
    if (group < 0 || group > 2) return;
    std::vector<std::pair<int, int>> &p = pool[group];
    if (cur >= 0 && cur < (int)p.size()) p.erase(p.begin() + cur);
    left[group]--;
}

// ---------------------------------------------------------------- the game
CloudBurstGame::CloudBurstGame() { questions(); }   // 0x478b10

CloudBurstGame::~CloudBurstGame() {}                 // 0x478d80 (the repeater, the pool, the mover)

// 0x47a290: a riddle added (its answers follow).
CloudQuestion *CloudBurstGame::addQuestion(const char *line, int kind, int param, int group) {
    CloudQuestion &e = q[nq];
    snprintf(e.line, sizeof e.line, "%s", line);
    e.kind = kind;
    e.param = param;
    pool.add(nq, group);                             // 0x471a20
    nq++;
    return &e;
}

// 0x478ed0: each animal's seven sprites (twice the size, hidden), then the riddles.
void CloudBurstGame::questions() {
    Raytrace::RayTrace &rt = starry_scene().rt;      // 0x5d2da0
    static const char *const an[5] = {"cat", "dog", "ele", "mou", "tig"};   // the table 0x47a270
    struct S { const char *suffix; Sprite3D **arr; };
    const S ss[7] = {{"poself", posel}, {"posert", poser}, {"huntlf", huntl}, {"huntrt", huntr},
                     {"fleelf", fleel}, {"fleert", fleer}, {"poof", poof}};
    for (int i = 0; i < 5; i++) {
        for (const S &x : ss) {
            char nm[64], pf[64];
            snprintf(nm, sizeof nm, "%s%s", an[i], x.suffix);   // 0x4f396f
            snprintf(pf, sizeof pf, "clou_ga_%s%s_", an[i], x.suffix);
            rt.addSprite(nm, pf, 1);                 // 0x42d9d0
            Sprite3D *s = rt.findSprite(nm);         // 0x42dc70
            s->load();                               // 0x42ef20
            s->scale = 300.0f;                       // +0x84
            hide(s);
            x.arr[i] = s;
        }
    }
    struct Q { const char *line; int kind, param, group; int n; int ans[5]; };
    static const Q qs[42] = {
        {"CLOU_VO_FOXflee", 2, 0, 0, 1, {1}},        {"CLOU_VO_FOXflee", 2, 3, 0, 1, {0}},
        {"CLOU_VO_FOXflee", 2, 2, 0, 1, {3}},        {"CLOU_VO_FOXhunt", 1, 0, 0, 1, {3}},
        {"CLOU_VO_FOXhunt", 1, 1, 0, 1, {0}},        {"CLOU_VO_FOXhunt", 1, 3, 0, 1, {2}},
        {"CLOU_VO_FOXbig", 3, 3, 0, 4, {0, 1, 4, 2}}, {"CLOU_VO_FOXbig", 3, 0, 0, 3, {1, 4, 2}},
        {"CLOU_VO_FOXbig", 3, 1, 0, 2, {4, 2}},      {"CLOU_VO_FOXbig", 3, 4, 0, 1, {2}},
        {"CLOU_VO_FOXsmall", 3, 2, 0, 4, {4, 1, 0, 3}}, {"CLOU_VO_FOXsmall", 3, 4, 0, 3, {1, 0, 3}},
        {"CLOU_VO_FOXsmall", 3, 1, 0, 2, {0, 3}},    {"CLOU_VO_FOXsmall", 3, 0, 0, 1, {3}},
        {"CLOU_VO_FOXcat", 3, 0, 1, 1, {4}},         {"CLOU_VO_FOXcat", 3, 4, 1, 1, {0}},
        {"CLOU_VO_FOXpurr", 0, -1, 1, 2, {0, 4}},    {"CLOU_VO_FOXzoo", 0, -1, 1, 2, {4, 2}},
        {"CLOU_VO_FOXhole", 0, -1, 1, 1, {3}},       {"CLOU_VO_FOXbig2", 0, -1, 1, 1, {2}},
        {"CLOU_VO_FOXsmall2", 0, -1, 1, 1, {3}},     {"CLOU_VO_FOXpickup", 0, -1, 2, 1, {2}},
        {"CLOU_VO_FOXclaws", 0, -1, 2, 1, {2}},      {"CLOU_VO_FOXbone", 0, -1, 2, 1, {1}},
        {"CLOU_VO_FOXfast", 0, -1, 2, 1, {4}},       {"CLOU_VO_FOXblind", 0, -1, 2, 1, {1}},
        {"CLOU_VO_FOXnana", 0, -1, 2, 1, {1}},       {"CLOU_VO_FOXpeanut", 0, -1, 2, 1, {2}},
        {"CLOU_VO_FOXcheese", 0, -1, 2, 1, {3}},     {"CLOU_VO_FOXtusk", 0, -1, 2, 1, {2}},
        {"CLOU_VO_FOXfur", 0, -1, 2, 3, {3, 1, 2}},  {"CLOU_VO_FOXnine", 0, -1, 2, 1, {0}},
        {"CLOU_VO_FOXbest", 0, -1, 2, 1, {1}},       {"CLOU_VO_FOXking", 0, -1, 2, 1, {4}},
        {"CLOU_VO_FOXmane", 0, -1, 2, 1, {4}},       {"CLOU_VO_FOXcanine", 0, -1, 2, 1, {1}},
        {"CLOU_VO_FOXcub", 0, -1, 2, 1, {4}},        {"CLOU_VO_FOXwag", 0, -1, 2, 1, {1}},
        {"CLOU_VO_FOXgray", 0, -1, 2, 1, {2}},       {"CLOU_VO_FOXleash", 0, -1, 2, 1, {1}},
        {"CLOU_VO_FOXwolf", 0, -1, 2, 1, {1}},       {"CLOU_VO_FOXmickey", 0, -1, 2, 1, {3}}};
    nq = 0;                                          // +0x98
    for (const Q &x : qs) {
        CloudQuestion *e = addQuestion(x.line, x.kind, x.param, x.group);
        for (int k = 0; k < x.n; k++) e->answers[e->count++] = x.ans[k];
    }
    state = 0;                                       // +0x1e8c
}

void CloudBurstGame::hideAll() {                     // 0x47a320
    hide(a);
    hide(c);
    hide(b);
    hide(d);
}

// 0x47a380 (state 3): once no line plays: the next riddle said (and repeated in silences), its side, the
// clicks held until it is heard; the answers kept out of Tink's hint.
void CloudBurstGame::ask() {
    busy = true;
    hide_cursor();                                   // 0x4119e0
    if (Game::state().vo_busy()) return;             // +0x560
    cur = pool.next();                               // 0x471e70
    repeater.start(q[cur].line);                     // 0x4c5fa0
    vo().play(q[cur].line);                          // 0x4c9400
    left_side = !(vision_random(10000) > 5000);      // 0x425d40 (+0x1ccc)
    state = q[cur].kind == 0 ? 12 : 4;
    chosen = -1;                                     // +0x1cc0
    for (int i = 0; i < 5; i++) {
        bool ans = false;
        for (int k = 0; k < q[cur].count; k++)
            if (q[cur].answers[k] == i) { ans = true; break; }
        asked[i] = ans ? 9999999 : 0;                // 0x98967f
    }
}

// 0x47a490 (state 4): the animal asked about standing in from its side; where it stops: 100 past the middle
// toward its side for "who does it run from", the other way for "who does it hunt".
void CloudBurstGame::showAnimal() {
    int p = q[cur].param;
    double sgn;
    if (left_side) {
        a = posel[p];                                // +0x04
        a->z = kLeftZ;
        sgn = 1.0f;                                  // 0x513710
    } else {
        a = poser[p];                                // +0x18
        a->z = kRightZ;
        sgn = -1.0f;                                 // 0x51522c
    }
    a->x = kStageX;
    a->y = kStageY;
    show(a);
    off = 0;                                         // +0x94
    if (q[cur].kind == 2) off = (float)(sgn * 100.0f);   // 0x513da0
    if (q[cur].kind == 1) off = (float)(sgn * -100.0f);  // 0x515228
    state = 5;
}

// 0x47a570: "three more", "one more", or one of four "nice"s.
void CloudBurstGame::praise() {
    if (pool.total() == 3) { vo().play("CLOU_VO_FOX3more"); return; }
    if (pool.total() == 1) { vo().play("CLOU_VO_FOX1more"); return; }
    static const char *const nice[4] = {"CLOU_VO_FOXnice1", "CLOU_VO_FOXnice2", "CLOU_VO_FOXnice3",
                                        "CLOU_VO_FOXnice4"};
    int r = vision_random(4);                        // the state's +8
    if (r >= 0 && r <= 3) vo().play(nice[r]);
}

// A sprite's puff where it stands (10 nearer the shelf), shown.
static Sprite3D *puff_at(Sprite3D *p, Sprite3D *s) {
    p->x = (float)((double)s->x - 10.0f);            // 0x514f48
    p->y = s->y;
    p->z = s->z;
    show(p);
    return p;
}

// 0x47a650 (state 8): a wrong answer: it walks in toward the middle (the other a little too), and both go in puffs.
void CloudBurstGame::wrong() {
    if (sub == 0) {
        if (left_side) {
            b = posel[chosen];
            b->z = kLeftZ;
        } else {
            b = poser[chosen];
            b->z = kRightZ;
        }
        b->x = kStageX;
        b->y = kStageY;
        show(b);
        sub = 1;
        return;
    }
    if (sub == 1) {
        double dd = (double)kMidZ - b->z;
        dist = (float)dd;                            // +0x1ce4
        if (fabs(dd) < 100.0) {                      // 0x5134c0
            sub = 2;
            return;
        }
        double step = dd > 0 ? 10.0f : -10.0f;       // 0x514f48, 0x515248
        b->z = (float)(step + b->z);
        a->z = (float)(step * 0.75f + a->z);         // 0x515244
    }
    if (sub == 2) {
        c = puff_at(poof[q[cur].param], a);          // +0x7c
        d = puff_at(poof[chosen], b);
        counter = 0;                                 // +0x1ce0
        sub = 3;
        return;
    }
    if (sub != 3) return;
    if (counter < c->anim->count() - 1) {            // 0x403c10
        if (counter == c->anim->count() - 2) {
            a->anim->visible = false;                // (the exe tests only the animations here)
            b->anim->visible = false;
        }
        c->anim->setFrame(counter);                  // 0x403f20
        d->anim->setFrame(counter);
        counter++;
        return;
    }
    hide(a);
    hide(c);
    hide(b);
    hide(d);
    state = 3;
}

// The answer appearing in a puff by the one asked about (at `dz` from it).
void CloudBurstGame_appear(CloudBurstGame *g, Sprite3D *s, double dz) {
    g->b = s;
    s->z = (float)((double)g->a->z + dz);
    s->x = kStageX;
    s->y = kStageY;
    g->d = puff_at(g->poof[g->chosen], s);
    g->counter = 0;
    g->sub = 1;
    starry_scene().fx[0]->play();                    // 0x5d7620 "clou_fx_appearpoof"
}

// The one asked about turned into `s` where it stands.
static Sprite3D *swap_to(Sprite3D *old, Sprite3D *s) {
    float x = old->x, y = old->y, z = old->z;
    hide(old);
    s->x = x;
    s->y = y;
    s->z = z;
    show(s);
    return s;
}

// The two run off side by side (20 a step, 500 in all); then the riddle answered.
static bool chase(CloudBurstGame *g) {
    if (g->dist < 500.0f) {                          // 0x515250
        g->dist = (float)((double)g->dist + 20.0f);  // 0x5150f8
        double step = g->left_side ? 20.0f : -20.0f; // 0x51524c
        g->a->z = (float)(step + g->a->z);
        g->b->z = (float)(step + g->b->z);
        return true;
    }
    return false;
}

// 0x47a8f0 (state 9): "who does it hunt" answered: the answer puffs in 200 on, takes fright, the other hunts it off.
void CloudBurstGame::huntRight() {
    if (sub == 0) {
        if (left_side) CloudBurstGame_appear(this, fleel[chosen], 200.0f);   // +0x54, 0x514340
        else CloudBurstGame_appear(this, fleer[chosen], -200.0f);           // +0x68
        return;
    }
    if (sub == 1) {
        if (counter < d->anim->count() - 1) {
            d->anim->setFrame(counter);
            counter++;
            return;
        }
        hide(d);
        show(b);
        int p = q[cur].param;
        a = swap_to(a, left_side ? huntl[p] : huntr[p]);   // +0x2c / +0x40
        dist = 0;
        sub = 2;
        starry_scene().fx[1 + p]->play();            // 0x5d7968 + 0x348 p: the hunter's "hunt"
        return;
    }
    if (sub != 2) return;
    if (chase(this)) return;
    pool.removeCurrent();                            // 0x472060
    hideAll();
    praise();                                        // 0x47a570
    state = 3;
}

// 0x47ac30 (state 10): "who does it run from" answered: the answer puffs in 200 back and hunts the other off.
void CloudBurstGame::fleeRight() {
    if (sub == 0) {
        if (left_side) CloudBurstGame_appear(this, huntl[chosen], -200.0f);   // +0x2c
        else CloudBurstGame_appear(this, huntr[chosen], 200.0f);             // +0x40
        return;
    }
    if (sub == 1) {
        if (counter < d->anim->count() - 1) {
            d->anim->setFrame(counter);
            counter++;
            return;
        }
        hide(d);
        show(b);
        int p = q[cur].param;
        a = swap_to(a, left_side ? fleel[p] : fleer[p]);   // +0x54 / +0x68
        dist = 0;
        sub = 2;
        starry_scene().fx[1 + chosen]->play();       // the hunter's "hunt"
        return;
    }
    if (sub != 2) return;
    if (chase(this)) return;
    pool.removeCurrent();
    praise();
    hideAll();
    state = 3;
}

// 0x47af60 (state 11): "bigger / smaller" answered: the answer puffs in 100 off, the two step together (5 each,
// 50 in all), and both go in puffs.
void CloudBurstGame::sizeRight() {
    if (sub == 0) {
        if (left_side) CloudBurstGame_appear(this, posel[chosen], -100.0f);   // +0x04, 0x513da0
        else CloudBurstGame_appear(this, poser[chosen], 100.0f);             // +0x18
        return;
    }
    if (sub == 1) {
        if (counter < d->anim->count() - 1) {
            d->anim->setFrame(counter);
            counter++;
            return;
        }
        hide(d);
        show(b);
        dist = 0;
        sub = 2;
        return;
    }
    if (sub == 2) {
        if (dist < 50.0f) {                          // 0x515190
            dist = (float)((double)dist + 5.0f);     // 0x514c84
            if (a->z > b->z) {
                a->z = (float)((double)a->z - 5.0f);
                b->z = (float)((double)b->z + 5.0f);
            } else {
                a->z = (float)((double)a->z + 5.0f);
                b->z = (float)((double)b->z - 5.0f);
            }
            return;
        }
        c = puff_at(poof[q[cur].param], a);
        d = puff_at(poof[chosen], b);
        counter = 0;
        sub = 3;
        return;
    }
    if (sub != 3) return;
    if (counter < c->anim->count() - 1) {
        if (counter == c->anim->count() - 2) {
            hide(a);
            hide(b);
        }
        c->anim->setFrame(counter);
        d->anim->setFrame(counter);
        counter++;
        return;
    }
    pool.removeCurrent();
    hideAll();
    praise();
    state = 3;
}

// The answer to an "any animal" riddle walking in from the side to the middle (20 a step) and going in a puff;
// true once gone.
static bool walk_and_puff(CloudBurstGame *g) {
    if (g->sub == 0) {
        if (g->left_side) {
            g->b = g->posel[g->chosen];
            g->b->z = kLeftZ;
        } else {
            g->b = g->poser[g->chosen];
            g->b->z = kRightZ;
        }
        g->b->x = kStageX;
        g->b->y = kStageY;
        show(g->b);
        g->sub = 1;
        return false;
    }
    if (g->sub == 1) {
        double dd = (double)kMidZ - g->b->z;
        g->dist = (float)dd;
        double step = dd > 0 ? 20.0f : -20.0f;       // 0x5150f8, 0x51524c
        g->b->z = (float)(step + g->b->z);
        if (fabs((double)g->dist) < 20.0) {          // 0x515258
            g->d = puff_at(g->poof[g->chosen], g->b);
            g->counter = 0;
            g->sub = 2;
        }
        return false;
    }
    if (g->sub != 2) return false;
    if (g->counter < g->d->anim->count() - 1) {
        if (g->counter == g->d->anim->count() - 2) g->b->anim->visible = false;
        g->d->anim->setFrame(g->counter);
        g->counter++;
        return false;
    }
    return true;
}

void CloudBurstGame::anyWrong() {                    // 0x47b300 (state 13)
    if (!walk_and_puff(this)) return;
    hideAll();
    state = 3;
}

void CloudBurstGame::anyRight() {                    // 0x47b510 (state 14)
    if (!walk_and_puff(this)) return;
    starry_scene().fx[0]->play();                    // 0x5d7620
    pool.removeCurrent();
    hideAll();
    praise();
    state = 3;
}

// 0x47b740: back to the area: its tune, the hot regions, moving again, Foxy's end of the game, Tink settled.
void CloudBurstGame::end() {
    StarrySkies_Scene &sc = starry_scene();
    sc.cloud_on = false;                             // 0x5d6ba1
    show_cursor();                                   // 0x4119d0
    sc.music(2);                                     // 0x478640
    sc.hotsOn(true);                                 // (inlined)
    sc.forceNone();                                  // 0x4abf30
    sc.clou_end.start();                             // 0x5d5850
    sc.tink->wanders = false;                        // 0x5d53a4 +0x13c
    sc.cloud_inactivity.stop();                      // 0x4c4690 (0x5d67f8)
    repeater.stop();                                 // 0x4c60f0
}

// 0x47b7c0: every riddle due answered: won (the star, then the cap, then nothing more), back to the area; the
// fuse out: lost. Then the state's step.
void CloudBurstGame::update() {
    StarrySkies_Scene &sc = starry_scene();
    State &st = Game::state();
    // (the exe's other branch, the game not on, reads an unset local: update runs only while it is on)
    if (sc.cloud_on && pool.total() == 0) {          // 0x472120
        if (sc.bits & 8) {                           // 0x5d6c34
            st.last_result = 4;                      // +0x7a0
        } else if (sc.bits & 4) {
            sc.bits |= 8;
            st.last_result = 3;
            sc.clou.cap->on = true;                  // 0x5d6bc0 +0x17c
        } else {
            sc.bits |= 4;
            st.last_result = 2;
            sc.clou.star->on = true;                 // 0x5d6bc4
        }
        end();                                       // 0x47b740
        hideAll();
        Game::cannon().hide();                       // 0x4e3bf0
        state = 1;
        return;
    }
    if (Game::cannon().visible && !Game::cannon().running()) {   // state +0x83c, 0x4e3b80
        state = 15;
        st.last_result = 0;
    }
    static int last = -1;                            // test aid
    if (getenv("TB_DEBUG_STARRY") && state != last) {
        tb_log("cloudburst: state %d (t=%u) left %d q %d (%s) chosen %d", state, frame_time(), pool.total(), cur,
               q[cur].line, chosen);
        last = state;
    }
    switch (state) {                                 // the table 0x47bbd0
    case 0: state = 1; break;
    case 1:                                          // the camera to the stage (spot 2) in 0.9 s
        if (sc.cloud_on) {
            float sp[4];
            st.cameraSpot(2, 2, sp);                 // 0x4ec180
            mover.start(&sc.rt, sp[0], sp[1], sp[2], sp[3], 900);   // 0x431690 (9 tenths)
            state = 2;
        }
        break;
    case 2:
        if (!mover.step()) state = 3;                // 0x431890
        break;
    case 3: ask(); break;
    case 4: showAnimal(); break;
    case 5: {                                        // the one asked about to its place (8 a step)
        double dd = ((double)off + kMidZ) - a->z;
        dist = (float)dd;
        if (fabs(dd) < 10.0) {                       // 0x515238
            state = 6;
            break;
        }
        a->z = (float)((dd > 0 ? 8.0f : -8.0f) + (double)a->z);   // 0x514f44, 0x515230
        break;
    }
    case 6:                                          // the riddle heard: clicks again
    case 12:
        if (!vo().isPlaying(q[cur].line) && busy) {  // 0x4c9690
            busy = false;
            show_cursor();
        }
        if (!busy && getenv("TB_CLOUD_AUTO")) choose(q[cur].answers[0]);   // test aid: the right answer
        break;
    case 7: {                                        // an answer clicked
        right = false;
        for (int k = 0; k < q[cur].count; k++)
            if (q[cur].answers[k] == chosen) { right = true; break; }
        if (right) {
            switch (q[cur].kind) {
            case 1: state = 9; break;
            case 2: state = 10; break;
            case 3: state = 11; break;
            default: break;
            }
        } else {
            state = 8;
        }
        sub = 0;
        break;
    }
    case 8: wrong(); break;
    case 9: huntRight(); break;
    case 10: fleeRight(); break;
    case 11: sizeRight(); break;
    case 13: anyWrong(); break;
    case 14: anyRight(); break;
    case 15:                                         // the fuse out
        hideAll();
        end();
        Game::cannon().hide();
        st.last_result = 0;
        state = 1;
        break;
    default: break;
    }
}

// 0x47bc10: not while busy; the animal asked about itself: "the same animal!"; else the answer (judged at once
// for an "any animal" riddle).
void CloudBurstGame::choose(int animal) {
    if (busy) return;
    if (q[cur].kind != 0 && animal == q[cur].param) {
        vo().play("CLOU_VO_sameANI");
        return;
    }
    chosen = animal;                                 // +0x1cc0
    busy = true;
    hide_cursor();
    if (state == 12) {
        right = false;
        for (int k = 0; k < q[cur].count; k++)
            if (q[cur].answers[k] == chosen) { right = true; break; }
        state = right ? 14 : 13;
        sub = 0;
    }
    if (state == 6) state = 7;
}

}  // namespace Game
