#include "state.h"
#include "../vision/engine.h"
#include "../vision/animation.h"
#include <stdio.h>
#include <stdlib.h>

namespace Game {

static const char kSection[] = "Tinkerbell Globals";   // *0x5434cc
static const char kFolder[] = "Save";                  // *0x5434d8
static const char kExt[] = "txt";                      // *0x5434dc

State &state() { static State s; return s; }

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

bool Saves::hasCap() { return false; }                   // 0x4e9af0 (port: TODO with the area progress)

}  // namespace Game
