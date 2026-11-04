// TokenInterface and HotRegion, reconstructed from "Tinker Bell.exe" (see tokens.h).
#include "tokens.h"
#include "state.h"
#include "voiceover.h"
#include <math.h>
#include <string.h>

namespace Game {

// ---------------------------------------------------------------- TokenInterface

TokenInterface::TokenInterface(const char *p)        // 0x4f0960
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), prefix(p) {}

// 0x4f0c00: the exe deletes the pieces and the backdrop (whatever the unload left)
TokenInterface::~TokenInterface() {
    for (Vision::Animation *&a : pieces) { delete a; a = nullptr; }
    delete back;
    back = nullptr;
}

void TokenInterface::add(const char *n, const char *p) {   // 0x4f0ec0
    names[count] = n;
    prefixes[count] = p;
    count++;
}

void TokenInterface::show(const char *n) {           // 0x4f1040
    for (int i = 0; i < count; i++)
        if (names[i] == n) { pieces[i]->visible = true; return; }   // +0x7c
}

// 0x4f0d10: the backdrop (shown) and the pieces (hidden), each a new animation, loaded.
void TokenInterface::load() {
    back = new Vision::Animation(prefix.c_str());    // 0x4243c0(0x118) + 0x404c20
    back->load();                                    // v15
    back->visible = true;
    for (int i = 0; i < count; i++) {
        pieces[i] = new Vision::Animation(prefixes[i].c_str());
        pieces[i]->load();
        pieces[i]->visible = false;
    }
}

// 0x4f0e10: the exe unloads them and forgets the pointers (they leak); port: deleted.
void TokenInterface::unload() {
    Graphic::unload();                               // 0x412020
    for (int i = 0; i < count; i++) {
        pieces[i]->unload();                         // v16
        delete pieces[i];
        pieces[i] = nullptr;
    }
    back->unload();
    delete back;
    back = nullptr;
}

// 0x4f0e60: only once a piece shows: the backdrop, then the pieces won (the exe walks five).
void TokenInterface::draw() {
    if (!visible) return;
    bool any = false;
    for (int i = 0; i < count; i++)
        if (pieces[i]->visible) any = true;
    if (!any) return;
    back->draw();                                    // v14
    for (int i = 0; i < 5; i++)
        if (pieces[i] && pieces[i]->visible) pieces[i]->draw();
}

// ---------------------------------------------------------------- HotRegion

HotRegion::HotRegion(Raytrace::RayTrace *r, const char *g, Raytrace::Sprite3D *spot, const char *l0,
                     const char *l1, const char *l2, const char *l3)
    : Task(nullptr), rt(r), x(spot->x), z(spot->z), game(g) {   // 0x4c3b30
    lines[0] = l0; lines[1] = l1; lines[2] = l2; lines[3] = l3;
}

// 0x4c3bf0: for a puzzle game, its area's saved pieces won (4 or 5; Skull Rock 5 or 6; the Indian village 4);
// 0 for any other game.
int HotRegion::won() {
    struct P { const char *game, *section; int a, b; };
    static const P ps[] = {
        {"Cloud Burst Game", "Coken Tokens", 4, 5},  // 0x4e9df0 (the exe's spelling)
        {"Shell Game", "Mermaid Tokens", 4, 5},      // 0x4e9e30
        {"Echo Echo Game", "Skull Tokens", 5, 6},    // 0x4e9e10
        {"Bucket Toss Game", "Lost Tokens", 4, 5},   // 0x4e9e20
        {"Totem Trail Game", "Village Tokens", 4, 4},   // 0x4e9e00
    };
    for (const P &p : ps) {
        if (strcmp(game, p.game) != 0) continue;
        int n = 0;
        for (const TokenRecord &r : Game::state().saves.tokens(p.section))
            if (r.state == p.a || r.state == p.b) n++;
        return n;
    }
    return 0;
}

bool HotRegion::inside() {
    if (!on) return false;
    double dx = (double)x - rt->camx, dz = (double)z - rt->camz;   // 0x42e400
    dist = (float)sqrt(dx * dx + dz * dz);
    return (double)dist < (double)radius;            // the x87 value against +0xd4
}

// 0x4c4200: on walking in (from outside last time), half a second, then up to 1.5 s more while staying; still
// there and nothing said: the line. Checked every second.
void HotRegion::run() {
    wait(1);
    ticks = 0;
    for (;;) {
        if (inside() && radius < last) {
            wait(500);
            while (inside()) {
                wait(500);
                if (++ticks == 3) break;
            }
            if (inside() && !Game::state().vo_busy()) {   // +0x560
                int info = Game::state().saves.gameInfo(game);   // 0x4e9d70
                const char *l = nullptr;
                if (info == 0) l = won() < 5 ? lines[0] : lines[1];
                else if (info == 1) l = lines[2];
                else if (info == 2) l = lines[3];
                if (l) Vision::VoiceOver::get().play(l);
            }
        }
        last = dist;
        ticks = 0;
        wait(1000);
    }
}

}  // namespace Game
