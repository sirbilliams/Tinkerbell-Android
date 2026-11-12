#include "state.h"
#include "../raytrace/raytrace.h"
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "voiceover.h"
#include "cannon.h"
#include "../vision/sound.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace Game {

static const char kSection[] = "Tinkerbell Globals";   // *0x5434cc
static const char kFolder[] = "Save";                  // *0x5434d8
static const char kExt[] = "txt";                      // *0x5434dc

State &state() { static State s; return s; }

// 0x4ebad0 (the state's +0x13a0): the camera spots {x, eye, z, degrees} of the 3D areas, five per area (as their
// float bit patterns; the unset ones are 0). London is area 0.
static uint32_t kSpots[30][4] = {   // (spot 4 of each area set by 0x4ec0e0)
    {0x4547b75cu, 0x3dcccccdu, 0x45155971u, 0x430a4ccdu},   // [0][0]
    {0x4547b75cu, 0x3dcccccdu, 0x45155971u, 0x430a4ccdu},   // [0][1]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [0][2]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [0][3]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [0][4]
    {0x457a0000u, 0x3dcccccdu, 0x44de8000u, 0x42dc0000u},   // [1][0]
    {0x45012fd3u, 0x3c23d70au, 0x457fe16du, 0x4305e1f6u},   // [1][1]
    {0x4580b8f6u, 0x3dcccccdu, 0x452f0f33u, 0x4322170au},   // [1][2]
    {0x458e72cdu, 0x3dcccccdu, 0x45a4bcb8u, 0x435eae98u},   // [1][3]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [1][4]
    {0x456d8000u, 0x3dcccccdu, 0x45034000u, 0x42480000u},   // [2][0]
    {0x45758bd7u, 0x3dcccccdu, 0x452edfaeu, 0x42d50083u},   // [2][1]
    {0x45bc4e14u, 0x3dcccccdu, 0x4528c23du, 0x43b30979u},   // [2][2]
    {0x45894e3du, 0x3dcccccdu, 0x44f2a852u, 0x3f7e9425u},   // [2][3]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [2][4]
    {0x454f635cu, 0x3dcccccdu, 0x44fd5052u, 0x4309ab85u},   // [3][0]
    {0x4587a4a4u, 0x3dcccccdu, 0x45364f33u, 0x433f1fbeu},   // [3][1]
    {0x457200f6u, 0x3dcccccdu, 0x448a870au, 0x4389ab64u},   // [3][2]
    {0x452578cdu, 0x3dcccccdu, 0x44f12385u, 0x43404d0eu},   // [3][3]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [3][4]
    {0x456d8000u, 0x3dcccccdu, 0x45034000u, 0x43070000u},   // [4][0]
    {0x45407214u, 0x3dcccccdu, 0x44d56143u, 0x4350aca0u},   // [4][1]
    {0x45916a3eu, 0x3c23d70au, 0x456f0438u, 0x428a2630u},   // [4][2]
    {0x45854733u, 0x3c23d70au, 0x44f1ea7fu, 0x40704b12u},   // [4][3]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [4][4]
    {0x453b819au, 0x3dcccccdu, 0x453b819au, 0x42b4185cu},   // [5][0]
    {0x454be852u, 0x3dcccccdu, 0x4571299au, 0x42ad4320u},   // [5][1]
    {0x4540c385u, 0x3dcccccdu, 0x451362e1u, 0x43717a5eu},   // [5][2]
    {0x45b5c852u, 0x3dcccccdu, 0x44e480a4u, 0x42ffab02u},   // [5][3]
    {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},   // [5][4]
};
void State::cameraSpot(int area, int spot, float out[4]) const {   // 0x4ec180
    memcpy(out, kSpots[area * 5 + spot], 16);
}
void State::setSpot4(int area, const float sp[4]) {   // 0x4ec0e0: the area's spot 4, made current
    memcpy(kSpots[area * 5 + 4], sp, 16);
    cur_spot[area] = 4;                              // +0x1e0
}
void State::setCameraToSpot(int area, int spot, Raytrace::RayTrace *rt) const {   // 0x4ec140
    float sp[4];
    cameraSpot(area, spot, sp);
    rt->setCamera(sp[0], sp[1], sp[2], sp[3]);       // 0x42a570
}
bool State::vo_busy() const { return Vision::VoiceOver::get().busy(); }

// 0x4ec920, called by the state's constructor: (re)opens the "global" pack (the cursors and the shared sounds'
// art) and creates the game's two cursor animations. port: TODO the rest of it (+0x68 = 1, the flags at +0x78,
// +0x3c4, +0x1598, +0x75/+0x76, and the four floats at +0x1718) with the scenes that use them.
void State::initGlobals() {
    if (global_ras) { Vision::ras_close(global_ras); global_ras = nullptr; }   // 0x41b050
    global_ras = Vision::ras_open("global", "", &Vision::data_device());       // 0x419b90
    global_ras->load();                                                          // 0x41aa20
    cursor = new Vision::Animation("glob_cu_normal_");                          // 0x404c20
    cursor_active = new Vision::Animation("glob_cu_active_");
}

// The end of the state's constructor 0x4ec6e0 (after 0x4ec920): the global pack current, the cursors loaded
// (the active one started, looping), the cannon and the voice-over loaded (its screen and keys active).
void State::finishInit() {
    skip_key = Vision::g_developer ? 0x51 : 0x1b;                               // +0x74 ('Q' in developer mode)
    Vision::ras_set_current(global_ras);                                       // 0x41a920
    cursor->load();                                                              // v15
    cursor_active->load();
    static_cast<Vision::Animation *>(cursor_active)->start_time = Vision::frame_time() + 1;   // 0x403be0(1, 1)
    static_cast<Vision::Animation *>(cursor_active)->mode = 1;
    if (!throwdust) throwdust = new Vision::Sound("GLOB_FX_throwdust");         // +0x7c (built by the ctor)
    Game::cannon().load();                                                       // +0x7c0 v15
    Vision::VoiceOver::get().load();                                             // +0x3e8 v15 (0x4c97f0)
}

Saves::Saves() {                                         // 0x4e9530
    globals = new Vision::Database("Globals", kExt, kFolder);   // *0x5434d4
    globals->load();
    open("save5");                                       // *0x5434e0
}

void Saves::open(const std::string &name) {              // 0x4e9680
    if (cur) {                                           // 0x4e4e30 x2 (both written), 0x4e3dc0 (closed)
        cur->write(); globals->write();
        delete cur;
    }
    cur = new Vision::Database(name, kExt, kFolder);     // 0x4e3c80 + 0x4e4b60 + 0x4e4980
    cur->load();
}

bool Saves::getInt(const char *key, int *out) {          // 0x4e9710
    const std::string *v = cur->get(kSection, key);
    if (v) *out = atoi(v->c_str());                      // 0x4242c0
    return v != nullptr;
}

void Saves::setInt(const char *key, int v) {             // 0x4e9760
    char b[16]; snprintf(b, sizeof b, "%d", v);          // "%d" (0x5329ac)
    cur->set(kSection, key, b);
}

bool Saves::getSkill(int *out) {                         // 0x4e97a0
    const std::string *v = cur->get(kSection, "Skill Level");
    if (v) { int n = atoi(v->c_str()); if (n > 0 && n < 4) *out = n; }
    return v != nullptr;
}

void Saves::setSkill(int v) { setInt("Skill Level", v); }   // 0x4e9800

void Saves::flush() { cur->write(); globals->write(); }  // 0x4e9840

void Saves::deleteCurrent() { cur->remove(); }           // 0x4e9860

bool Saves::flag(Vision::Database *db, const char *key, bool *out) {   // 0x4ea4f0 / 0x4eae10 pattern
    const std::string *v = db->get(kSection, key);
    if (v) *out = !(v->empty() || *v == "0");
    return v != nullptr;
}

bool Saves::tutorialsPassed(bool *out) { return flag(globals, "Tutorials Passed", out); }

void Saves::setTutorialsPassed(bool v) {                 // 0x4ea570
    globals->set(kSection, "Tutorials Passed", v ? "true" : "");
}

bool Saves::hasTreasure(bool *out) { return flag(cur, "Has Treasure", out); }   // 0x4eae10

// 0x4e9af0: every one of the fifteen mini games won twice ("Info" 2 in each game's section; 0x4e96d0 each).
bool Saves::hasCap() {
    static const char *const games[15] = {           // *0x5434fc..*0x543534
        "Bird Spy Game", "Color The Cloud Game", "Cloud Burst Game", "Backwards Go Game", "Pearl Toss Game",
        "Shell Game", "Shadow Boxing Game", "Echo Echo Game", "Firefly Game", "Mud Pie Game",
        "Coconuts Away Game", "Flying Hoops Game", "Totem Trail Game", "Pirate Flag Tag Game", "Bucket Toss Game"};
    for (const char *g : games) {
        const std::string *v = cur->get(g, "Info");
        if (!v || atoi(v->c_str()) != 2) return false;
    }
    return true;
}

bool Saves::mapSegment(const char *area) {           // 0x4ea870 & co: *0x543538 "Map Segment"
    return cur->get(std::string(area) + " Treasure Map", "Map Segment") != nullptr;
}

bool Saves::anyMapSegment() {                        // 0x4eabb0
    static const char *const areas[5] = {"Mermaid", "Village", "Skull", "Lost", "Starry"};
    for (const char *a : areas)
        if (mapSegment(a)) return true;
    return false;
}

void Saves::setMapSegment(const char *area) {
    cur->set(std::string(area) + " Treasure Map", "Map Segment", "true");
}

bool Saves::allMapSegments() {                       // 0x4eaa70
    static const char *const areas[5] = {"Mermaid", "Village", "Skull", "Lost", "Starry"};
    for (const char *a : areas)
        if (!mapSegment(a)) return false;
    return true;
}

int Saves::gameInfo(const char *game) {                  // 0x4e9d70
    const std::string *v = cur->get(game, "Info");      // *0x5434e4
    return v ? atoi(v->c_str()) : 0;
}

}  // namespace Game

namespace Game {

// ---------------------------------------------------------------- the areas' puzzle pieces

void Saves::setGameInfo(const char *game, int v) {   // 0x4e9db0
    char b[16]; snprintf(b, sizeof b, "%d", v);      // "%d"
    cur->set(game, "Info", b);                       // *0x5434e4
}

// 0x4e9e40: the five pieces of "<Area> Tokens" ("location<n>" (the state), "active<n>" (easy), "x<n>"...). A key
// that is missing keeps the value read for the piece before.
std::vector<TokenRecord> Saves::tokens(const char *sec) {
    std::vector<TokenRecord> v;
    Vision::Database::Section *s = cur->section(sec, true);   // 0x4e4fb0
    if (s->entries.empty()) return v;                // +0x28
    TokenRecord r;
    for (int i = 0; i < 5; i++) {
        char n[16]; snprintf(n, sizeof n, "%d", i);  // "%d"
        const std::string *e;
        if ((e = cur->get(sec, std::string("location") + n))) r.state = atoi(e->c_str());   // 0x4242c0
        if ((e = cur->get(sec, std::string("active") + n))) r.easy = !(e->empty() || *e == "0");   // 0x4eb120
        if ((e = cur->get(sec, std::string("x") + n))) r.x = (float)atof(e->c_str());   // 0x4f595c
        if ((e = cur->get(sec, std::string("y") + n))) r.y = (float)atof(e->c_str());
        if ((e = cur->get(sec, std::string("z") + n))) r.z = (float)atof(e->c_str());
        v.push_back(r);                              // 0x4eaee0
    }
    return v;
}

void Saves::setTokens(const char *sec, const std::vector<TokenRecord> &v) {   // 0x4ea1b0
    for (int i = 0; i < 5 && i < (int)v.size(); i++) {
        char n[16], b[64];
        snprintf(n, sizeof n, "%d", i);
        const TokenRecord &r = v[i];
        snprintf(b, sizeof b, "%d", r.state);
        cur->set(sec, std::string("location") + n, b);
        cur->set(sec, std::string("active") + n, r.easy ? "true" : "");   // 0x4eb140
        snprintf(b, sizeof b, "%f", (double)r.x);    // "%f"
        cur->set(sec, std::string("x") + n, b);
        snprintf(b, sizeof b, "%f", (double)r.y);
        cur->set(sec, std::string("y") + n, b);
        snprintf(b, sizeof b, "%f", (double)r.z);
        cur->set(sec, std::string("z") + n, b);
    }
}

// 0x4ecbf0: "hiding_places.dat" [area]: "easy" and "hard", each with places whose first three children are x, y
// and z (by position).
void HidingPlaces::load(const char *area) {
    Vision::Database db("hiding_places", "dat", "");  // 0x4e3c80 (cached by the exe), loaded once (0x4e4980)
    db.load();
    Vision::Database::Section *s = db.section(area, true);
    auto read = [](const Vision::Database::Entry &e, P &p) {
        const std::vector<Vision::Database::Entry> &c = e.children;
        p.x = c.size() > 0 ? (float)atof(c[0].value.c_str()) : 0;   // 0x4f595c
        p.y = c.size() > 1 ? (float)atof(c[1].value.c_str()) : 0;
        p.z = c.size() > 2 ? (float)atof(c[2].value.c_str()) : 0;
    };
    for (const Vision::Database::Entry &e : s->entries) {
        if (e.key != "easy") continue;               // 0x424ea0 (the first one)
        for (const Vision::Database::Entry &h : e.children) { P p; read(h, p); easy.push_back(p); }   // 0x4f1150
        break;
    }
    for (const Vision::Database::Entry &e : s->entries) {
        if (e.key != "hard") continue;
        for (const Vision::Database::Entry &h : e.children) { P p; read(h, p); hard.insert(hard.begin(), p); }
        break;
    }
}

void State::Firsts::welcome() {                      // 0x4c64d0
    State &st = state();
    if (st.last_game == 2 || st.last_game == 5) return;
    Vision::Scene *sc = Vision::current_scene();     // 0x5a7394
    std::string n = sc ? sc->name() : "";
    if (n == "map") return;
    struct A { const char *scene, *seg, *first, *seg_won, *other; };
    static const A areas[5] = {                      // 0x631750.., 0x542538..0x542570
        {"Mermaid", "Mermaid", "MCOV_VO_PETenter1", "MCOV_VO_PETenter2", "MCOV_VO_PETremind"},
        {"Indian_Village", "Village", "INDV_VO_PETenter1", "INDV_VO_PETenter2", "INDV_VO_PETremind"},
        {"skull", "Skull", "SKLR_VO_PETenter1", "SKLR_VO_PETenter2", "SKLR_VO_PETremind"},
        {"lost", "Lost", "LOST_VO_PETenter1", "LOST_VO_PETenter2", "LOST_VO_PETremind"},
        {"starry", "Starry", "STAR_VO_PETenter1", "STAR_VO_PETenter2", "STAR_VO_PETremind"},
    };
    bool found = false;
    for (int i = 0; i < 5; i++) {
        if (n != areas[i].scene) continue;
        if (b[i]) { line = areas[i].first; b[i] = 0; }
        else line = st.saves.mapSegment(areas[i].seg) ? areas[i].seg_won : areas[i].other;
        found = true;
        break;
    }
    if (!found && n == "demo") return;
    Vision::VoiceOver::get().play(line.c_str());             // 0x4c9400
}

bool State::Firsts::welcoming() { return Vision::VoiceOver::get().isPlaying(line.c_str()); }   // 0x4c6930

bool hiding_sprite(const std::string &area, Raytrace::Sprite3D *s) {   // 0x4e6850
    Vision::Database db("hiding_places", "dat", "");
    db.load();
    Vision::Database::Section *sec = db.section(area, true);
    for (const Vision::Database::Entry &e : sec->entries) {
        if (e.key != "hidingSprites") continue;      // 0x424f90 (the first one)
        for (const Vision::Database::Entry &h : e.children)
            if (h.key == s->anim->name()) return true;   // 0x4243f0, 0x423840
        return false;
    }
    return false;
}

bool hides_near(float x, float z, Raytrace::Sprite3D *s) {   // 0x4e69f0
    Vision::Database db("hiding_places", "dat", "");
    db.load();
    const std::string *v = db.get("global", "hiding_radius");
    float r = (float)atof(v ? v->c_str() : "");      // 0x4f595c
    double dx = (double)s->x - x, dz = (double)s->z - z;
    return (double)r * r > dx * dx + dz * dz;
}

// 0x4f1510, MSVC's random_shuffle: each element from the second swapped with one at or before it.
static void shuffle(std::vector<HidingPlaces::P> &v) {
    for (size_t i = 1; i < v.size(); i++) {
        uint32_t r = (uint32_t)Vision::crt_rand();   // (& 0x7fff; the widening for longer lists never applies)
        size_t j = r % (i + 1);
        std::swap(v[i], v[j]);
    }
}

// 0x4ecf90: both lists shuffled the first time; then the last hard place while fewer than `skill` were handed
// out, else the last easy one.
void HidingPlaces::assign(TokenInfo *t, int skill) {
    if (!shuffled) {
        if (!easy.empty()) shuffle(easy);
        if (!hard.empty()) shuffle(hard);
        shuffled = true;
    }
    if (given < skill && !hard.empty()) {
        P p = hard.back();
        t->setPos(p.x, p.y, p.z);
        t->setHard();
        hard.pop_back();
        given++;
        return;
    }
    P p = easy.back();                               // port: the exe reads before the start of an empty list
    t->setPos(p.x, p.y, p.z);
    t->setEasy();
    easy.pop_back();
    given++;
}

// 0x4ee860: the saved pieces; a new player's: three hidden in the Lost Boys' area, one each from Pirate Flag Tag
// (2) and Mud Pie (3), at random.
void State::loadLostTokens() {
    std::vector<TokenRecord> v = saves.tokens("Lost Tokens");   // 0x4e9e20
    if (!v.empty()) {
        for (size_t i = 0; i < v.size() && i < 5; i++) {
            lost[i].state = v[i].state;
            if (v[i].easy) lost[i].setEasy(); else lost[i].setHard();
            lost[i].x = v[i].x;
            lost[i].y = v[i].y;
            lost[i].z = v[i].z;
        }
        return;
    }
    for (TokenInfo &t : lost) t.state = 0;
    HidingPlaces hp;                                 // 0x4f1110
    hp.load("lost");                                 // 0x4ecbf0
    for (int n = 0; n < 3; n++) {
        int r;
        do r = Vision::vision_random(5); while (lost[r].state != 0);   // the local random object (0x425d40)
        lost[r].state = 1;
        hp.assign(&lost[r], skill);                  // +0x68
    }
    int r;
    do r = Vision::vision_random(5); while (lost[r].state != 0);
    lost[r].state = 2;
    for (TokenInfo &t : lost)
        if (t.state == 0) { t.state = 3; break; }
    saveLostTokens();
}

void State::saveLostTokens() {                       // 0x4eec40
    std::vector<TokenRecord> v(5);
    for (int i = 0; i < 5; i++) {
        v[i].state = lost[i].state;
        if (lost[i].state != 1) continue;            // only a hidden piece keeps its place (the others: the
                                                     // exe's uninitialised local; port: zeros)
        v[i].easy = lost[i].easy;
        v[i].x = lost[i].x;
        v[i].y = lost[i].y;
        v[i].z = lost[i].z;
    }
    saves.setTokens("Lost Tokens", v);               // 0x4ea170
}

// 0x4ed270: Indian Village's pieces ("Village Tokens", 0x4e9e00); a new player's: three hidden in the area (state
// 3), one from Bird Spy (1) and one from Flying Hoops (2), at random. (The saved slot is not kept.)
void State::loadVillageTokens() {
    std::vector<TokenRecord> v = saves.tokens("Village Tokens");   // 0x4e9e00 -> 0x4e9e40 (*0x5434e8)
    if (!v.empty()) {
        // port: the exe writes as many records as were read (the save holds five)
        for (size_t i = 0; i < v.size() && i < 5; i++) {
            village[i].state = v[i].state;
            if (v[i].easy) village[i].setEasy(); else village[i].setHard();
            village[i].x = v[i].x;
            village[i].y = v[i].y;
            village[i].z = v[i].z;
        }
        return;
    }
    for (VillageToken &t : village) t.state = 0;
    HidingPlaces hp;                                 // 0x4f1110
    hp.load("indian_village");                       // 0x4ecbf0
    for (int n = 0; n < 3; n++) {
        int r;
        do r = Vision::vision_random(5); while (village[r].state != 0);   // 0x425d40
        village[r].state = 3;
        TokenInfo t;                                 // port: 0x4ecf90 calls the piece's v1 / v2 / v3 (another class)
        hp.assign(&t, skill);                        // 0x4ecf90 (+0x68)
        village[r].setPos(t.x, t.y, t.z);
        if (t.easy) village[r].setEasy(); else village[r].setHard();
    }
    int r;
    do r = Vision::vision_random(5); while (village[r].state != 0);
    village[r].state = 1;
    for (VillageToken &t : village)
        if (t.state == 0) { t.state = 2; break; }
    saveVillageTokens();
}

void State::saveVillageTokens() {                    // 0x4ed6b0
    std::vector<TokenRecord> v(5);
    for (int i = 0; i < 5; i++) {
        v[i].state = village[i].state;
        if (village[i].state != 3) continue;         // (the others: the exe's uninitialised local; port: zeros)
        v[i].easy = village[i].easy;
        v[i].x = village[i].x;
        v[i].y = village[i].y;
        v[i].z = village[i].z;
    }
    saves.setTokens("Village Tokens", v);            // 0x4ea130
}

// 0x4ed9c0: Starry Skies' pieces ("Coken Tokens", 0x4e9df0); a new player's: three hidden in the area (state 3),
// one each from Color the Clouds (2) and Backwards Go (1), at random.
void State::loadStarryTokens() {
    std::vector<TokenRecord> v = saves.tokens("Coken Tokens");   // 0x4e9df0 -> 0x4e9e40 (*0x5434f8)
    if (!v.empty()) {
        for (size_t i = 0; i < v.size() && i < 5; i++) {
            starry[i].state = v[i].state;
            if (v[i].easy) starry[i].setEasy(); else starry[i].setHard();
            starry[i].x = v[i].x;
            starry[i].y = v[i].y;
            starry[i].z = v[i].z;
        }
        return;
    }
    for (TokenInfo &t : starry) t.state = 0;
    HidingPlaces hp;                                 // 0x4f1110
    hp.load("starry");                               // 0x4ecbf0
    for (int n = 0; n < 3; n++) {
        int r;
        do r = Vision::vision_random(5); while (starry[r].state != 0);   // 0x425d40
        starry[r].state = 3;
        hp.assign(&starry[r], skill);                // 0x4ecf90 (+0x68)
    }
    int r;
    do r = Vision::vision_random(5); while (starry[r].state != 0);
    starry[r].state = 2;
    for (TokenInfo &t : starry)
        if (t.state == 0) { t.state = 1; break; }
    saveStarryTokens();
}

void State::saveStarryTokens() {                     // 0x4edd90
    std::vector<TokenRecord> v(5);
    for (int i = 0; i < 5; i++) {
        v[i].state = starry[i].state;
        if (starry[i].state != 3) continue;          // only a hidden piece keeps its place (the others: the
                                                     // exe's uninitialised local; port: zeros)
        v[i].easy = starry[i].easy;
        v[i].x = starry[i].x;
        v[i].y = starry[i].y;
        v[i].z = starry[i].z;
    }
    saves.setTokens("Coken Tokens", v);              // 0x4ea170
}

}  // namespace Game

namespace Game {
static void drop_numbers(State &s) {
    for (int i = 0; i < 10; i++) {
        delete s.num_big[i];                         // +0xc (deleting dtor)
        s.num_big[i] = nullptr;
        delete s.num_small[i];
        s.num_small[i] = nullptr;
    }
}

// 0x4eca30: the pack (re)opened, the twenty number images made from it and loaded.
void State::openGlobal2d() {
    if (ras2d) {
        Vision::ras_close(ras2d);                    // 0x41b050
        ras2d = nullptr;
        drop_numbers(*this);
    }
    ras2d = Vision::ras_open("global_2d", nullptr, &Vision::data_device());   // 0x419b90
    ras2d->load();                                   // 0x41aa20
    char buf[32];
    for (int i = 0; i < 10; i++) {
        snprintf(buf, sizeof buf, "glob_if_num%1.1dbg_01", i);   // 0x4f396f
        num_big[i] = Gorilla::gnew<Vision::Image>(buf, 0, 0, 0, 0, 0, 0, true);   // 0x4179e0
        snprintf(buf, sizeof buf, "glob_if_num%1.1dsm_01", i);
        num_small[i] = Gorilla::gnew<Vision::Image>(buf, 0, 0, 0, 0, 0, 0, true);
    }
    Vision::ras_set_current(ras2d);                  // 0x41a920
    for (int i = 0; i < 10; i++) {
        num_big[i]->load();                          // v15
        num_small[i]->load();
    }
}

void State::closeGlobal2d() {                        // 0x4ecba0
    if (!ras2d) return;
    Vision::ras_close(ras2d);
    ras2d = nullptr;
    drop_numbers(*this);
}
}  // namespace Game

namespace Game {
static int game_first_index() {
    static const char *const games[5] = {"coconut", "birdspy", "mudpie", "colorthecloud", "firefly"};
    Vision::Scene *s = Vision::current_scene();      // 0x5a7394
    if (!s) return -1;
    for (int i = 0; i < 5; i++)
        if (strcmp(s->name(), games[i]) == 0) return 5 + i;
    return -1;
}
bool State::Firsts::gameFirst() {                    // 0x4c6970
    int i = game_first_index();
    return i >= 0 ? b[i] != 0 : false;
}
void State::Firsts::gameDone() {                     // 0x4c6af0
    int i = game_first_index();
    if (i >= 0) b[i] = 0;
}
}  // namespace Game
