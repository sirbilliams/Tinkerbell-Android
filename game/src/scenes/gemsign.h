// GemSign (0x30 bytes, no vtable; 0x4e6400..0x4e65b0), reconstructed from "Tinker Bell.exe": a sign board of the
// 3D scenes with the tokens won on it (London's, the Lost Boys' three).
#pragma once
#include "../raytrace/raytrace.h"
#include "../vision/sound.h"

namespace Game {

// The sign board with the player's tokens (0x30 bytes at +0x3508; 0x4e6400 ctor, 0x4e6440 init, 0x4e6530 release,
// 0x4e6570 show, 0x4e6590 hide, 0x4e65b0 mouse): the "AA_lond_gs_sign_01" sprite of the street and four attachments
// on it: the tick, the green highlight, the cap and the star.
struct GemSign {
    GemSign(const char *sign, const char *tick, const char *green, const char *cap, const char *star);   // 0x4e6400
    void init(Raytrace::RayTrace *rt);               // 0x4e6440
    void release();                                  // 0x4e6530
    void show();                                     // 0x4e6570: its animation shown, its box back
    void hide();                                     // 0x4e6590
    bool mouse();                                    // 0x4e65b0: the highlight under the mouse; a click plays the click
    const char *names[5];                            // +0x00..+0x10
    Raytrace::Sprite3D *sprite = nullptr;            // +0x14
    Raytrace::Attachment *green = nullptr, *cap = nullptr, *star = nullptr;   // +0x18 +0x1c +0x20
    Raytrace::RayTrace *rt = nullptr;                // +0x24
    bool enabled = false;                            // +0x28
    bool clicked = false;                            // +0x29
    Vision::Sound *click = nullptr;                  // +0x2c "GLOB_FX_GSclick"
};

}  // namespace Game
