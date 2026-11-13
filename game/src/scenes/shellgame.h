// The Shell Game, the skunk's puzzle game in Mermaid Cove (played in the scene itself on the wooden log; the game
// is the scene's +0x44a8), reconstructed from "Tinker Bell.exe" (vtable 0x5149c0, 0x13a8 bytes; ctor 0x456750,
// setup 0x4569a0, start 0x456ad0, stop 0x456cf0, run v10 0x456e70, notify2 v9 0x456fe0, shellOf 0x457910,
// check 0x4579d0, hint 0x457b00, pick 0x4564d0). The five shells won lie in the log's left row; each goes into one
// of the five holes of the right row, in an order picked at random. The button checks: right, the game is won
// (the map segment); wrong, the misplaced shells go back to the left.
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
#include "../vision/sound.h"
#include "../raytrace/raytrace.h"
#include "../raytrace/mover.h"
#include "voacts.h"
#include "acts.h"
#include <vector>

namespace Game {

class ShellGame : public Vision::Task {
public:
    explicit ShellGame(Raytrace::RayTrace *rt);      // 0x456750
    void setup();                                    // 0x4569a0 (enter): the log's attachments, the shells won shown
    void start();                                    // 0x456ad0
    void stop();                                     // 0x456cf0
    void run() override;                             // v10 0x456e70
    void notify2(Vision::Service *s) override;       // v9 0x456fe0
    int shellOf(Raytrace::Attachment *a);            // 0x457910: which shell it shows (5: none)
    bool check();                                    // 0x4579d0: all five right (and the game's end set)
    bool hint(Raytrace::Attachment **out);           // 0x457b00: a misplaced shell's hole, the least shown one
    int heldShell();                                 // (inlined): the shell on the cursor (5: none)
    Vision::Graphic *cursorOf(int shell);            // (inlined): &cursors[shell]
    void putLeft(Raytrace::Attachment *a, int shell);   // (inlined): the left hole shows that shell, kept in place
    Raytrace::RayTrace *rt;                          // +0xc0
    Raytrace::Attachment *button = nullptr;          // +0xc4 "shel_ga_button_"
    Raytrace::Attachment *left[5] = {};              // +0xc8 "LT<n>" "SHELL_GA_target<n>L_"
    Raytrace::Attachment *right[5] = {};             // +0xdc "PT<n>" "SHELL_GA_target<n>_"
    Vision::Animation *cursors[5] = {};              // +0xf0 (0x118 each) "SHEL_CU_puzshell<n>_"
    Vision::Mouse mouse;                             // +0x668
    Vision::Keyboard keys;                           // +0x69c
    std::vector<int> order;                          // +0x6d0: the shell each right hole wants
    bool button_on = false;                          // +0x6e0: all five holes filled (the button animates)
    Raytrace::CameraTween mover;                     // +0x6e8
    int info = 0;                                    // +0x7f0
    Vision::Sound grab;                              // +0x7f4 "shel_fx_grabshell"
    Inactivity inactivity;                           // +0xb3c "SHEL_VO_SKNprompt"
    Vision::Sound drop;                              // +0xedc "SHEL_FX_shelldrop"
    int f1224 = 0;                                   // +0x1224
    int hinted[5] = {};                              // +0x1228: a misplaced shell's hints (9999999: right or empty)
    Vision::VoList wrong;                            // +0x123c: notright twice, mixup
    Vision::VoList nice;                             // +0x1270
    Vision::PlayVos nice_vos;                        // +0x12a4 (every 12 s)
    Raytrace::Sprite3D *log = nullptr;               // +0x1370 "AA_shel_ga_woodlog_01"
};

}  // namespace Game
