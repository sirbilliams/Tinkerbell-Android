#include "state.h"
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
static const uint32_t kSpots[30][4] = {
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

bool Saves::hasCap() { return false; }

int Saves::gameInfo(const char *game) {                  // 0x4e9d70
    const std::string *v = cur->get(game, "Info");      // *0x5434e4
    return v ? atoi(v->c_str()) : 0;
}                   // 0x4e9af0 (port: TODO with the area progress)

}  // namespace Game
