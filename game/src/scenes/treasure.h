// The treasure (the map's X spot, scene "treasure"), reconstructed from "Tinker Bell.exe": Treasure_Scene (vtable
// 0x5158b4, ctor 0x48f660, dtor 0x490630, the static at 0x5f6d50). Peter and Wendy find a door with a tile puzzle
// (WamiMovie); Peter gives the clues one at a time ("WAMI_VO_PETclue<n>", an "H" one when hard) and the player puts
// the right tile of the sixteen in each of the five slots; a wrong set starts over, the right one opens the door
// (WamiMovie3) and the ending plays (WamiMovie2): back to the sign-in room.
//   load2 v18 0x490930, enter v19 0x490980, leave v20 0x490fe0, run v10 0x491130, notify2 v9 0x491a20,
//   onMouse v25 0x491610, onKey v26 0x491580; makePuzzle 0x490e40, click 0x4916b0, pick 0x491790, place 0x4918f0,
//   step 0x491a60, gridShow 0x491b80, answerShow 0x491c50, held 0x491db0, clue 0x491e70, bellHelp 0x491f60.
//   Names: answer 0x48f090 ("answer%i%2.2d"), isGrid 0x48f1a0, isAnswer 0x48f2c0, tile 0x48f430
//   ("WAMI_GA_tilep_%2.2d", from 1), grid 0x48f530 ("grid%2.2d").
#pragma once
#include "raytrace_scene.h"
#include "wamimovie.h"
#include "acts.h"
#include "voacts.h"
#include "../vision/sound.h"

namespace Game {

class Treasure_Scene : public TinkerbellRaytraceScene {
public:
    explicit Treasure_Scene(const char *name);       // 0x48f660 ("treasure")
    ~Treasure_Scene() override;                      // 0x490630
    void load2() override;                           // v18 0x490930
    void enter() override;                           // v19 0x490980
    void leave() override;                           // v20 0x490fe0
    void run() override;                             // v10 0x491130
    void notify2(Vision::Service *s) override;       // v9 0x491a20
    void onMouse() override;                         // v25 0x491610
    bool onKey() override;                           // v26 0x491580
    void makePuzzle();                               // 0x490e40
    void click();                                    // 0x4916b0
    void pick();                                     // 0x491790 (state 0)
    void place();                                    // 0x4918f0 (state 1)
    void step();                                     // 0x491a60 (state 0, each frame)
    void gridShow(int i, bool on);                   // 0x491b80
    void answerShow(int k, int j, bool on);          // 0x491c50: on, tile j in slot k; off, slot k empty
    void held(bool on);                              // 0x491db0: the held tile on the cursor, or the plain cursor
    void clue();                                     // 0x491e70
    void bellHelp();                                 // 0x491f60
    void setTileCursor(int tile);                    // (inlined: "wami_cu_tile%02d_01" on the cursor)

    Vision::Image tile_cursor;                       // +0x2920
    int state = 0;                                   // +0x28e8: 0 picking, 1 holding a tile, 2 won, 3 wrong
    int solution[5] = {};                            // +0x28ec: the tile each slot wants
    bool hard[5] = {};                               // +0x2900 (5 bytes)
    int slots[5] = {-1, -1, -1, -1, -1};             // +0x2908: the tile put in each slot
    uint32_t clue_t = 0;                             // +0x291c
    int held_tile = 0;                               // +0x29c8
    int next = 0;                                    // +0x29cc: the first empty slot (-1 none)
    WamiMovie movie;                                 // +0x29d0
    WamiMovie2 movie2;                               // +0x3e20
    WamiMovie3 movie3;                               // +0x4ae0
    int rows[4] = {};                                // +0x4e58: the clue's row marked (never read)
    int last_clue = 9999999;                         // +0x4e68
    Vision::Ras *tnk_pack = nullptr;                 // +0x4f78 "WAMI_TNK"
    bool playing = false;                            // +0x4f7c
    PromptLine prompt;                               // +0x4f80 "WAMI_VO_PETprompt"
    VoOneAfterAnother wrong;                         // +0x5088 "WAMI_VO_HOKtoobad", "WAMI_VO_PETagain"
    Vision::Sound *tada, *opmusic, *music;           // +0x515c +0x54a4 +0x57ec (embedded)
    Vision::Ras *packs[6] = {};                      // +0x5b34..+0x5b48
};

Treasure_Scene &treasure_scene();                    // the static at 0x5f6d50

}  // namespace Game
