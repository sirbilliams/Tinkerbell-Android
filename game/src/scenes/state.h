// The game's global state object (0x1840 bytes, created by My_Stage +0x10 0x4e72d0 -> ctor 0x4ec6e0, pointer at
// 0x63171c). Only the parts the scenes ported so far use are here; the rest comes with those scenes.
#pragma once
#include "../vision/database.h"
#include <string>
namespace Vision { class Graphic; class Ras; }

namespace Game {

// state+0x3cc (0x4e9530): the save databases. `cur` = the player's save (Save\save<N>.txt; "save5" at start),
// `globals` = Save\Globals.txt. Section "Tinkerbell Globals" (*0x5434cc), folder "Save" (*0x5434d8), ext "txt".
class Saves {
public:
    Saves();                                         // 0x4e9530
    void open(const std::string &name);              // 0x4e9680: writes the old one out, opens <name>
    bool getInt(const char *key, int *out);          // 0x4e9710 ("Character", "Location": atoi of the value)
    void setInt(const char *key, int v);             // 0x4e9760 ("%d")
    bool getSkill(int *out);                         // 0x4e97a0 ("Skill Level": true if the key exists; *out set for 1..3)
    void setSkill(int v);                            // 0x4e9800
    void flush();                                    // 0x4e9840: writes both files
    void deleteCurrent();                            // 0x4e9860: deletes the current save file
    bool tutorialsPassed(bool *out);                 // 0x4ea4f0 (globals: "Tutorials Passed")
    void setTutorialsPassed(bool v);                 // 0x4ea570 ("true" / "")
    bool hasTreasure(bool *out);                     // 0x4eae10 ("Has Treasure")
    bool hasCap();                                   // 0x4e9af0 (port: TODO, reads every area's progress)
private:
    static bool flag(Vision::Database *db, const char *key, bool *out);
    Vision::Database *cur = nullptr, *globals = nullptr;
};

struct State {
    void finishInit();                               // the rest of 0x4ec6e0: loads the cursors, cannon, voice-over
    void initGlobals();                              // 0x4ec920 (part): the "global" pack and the game cursors
    Vision::Ras *global_ras = nullptr;               // +0x0
    Vision::Graphic *cursor_active = nullptr;        // +0x60 ("glob_cu_active_")
    Vision::Graphic *cursor = nullptr;               // +0x5c ("glob_cu_normal_"): the game's own cursor (port: created with the state object
                                                     // 0x4ec6e0, not ported yet; null = none)
    bool paused3d = false;                           // +0x64: the 3D scenes' frame work stops (Global3DTask)
    int skill = 0;                                   // +0x68
    bool hover_up = false, hover_down = false;       // +0x75 +0x76: the 3D eye rises to 200 / stays
    bool enter_released = true;                      // +0x77
    Saves saves;                                     // +0x3cc
    // +0x560: a voice line is playing (the byte is the VoiceOver member's +0x178 flag, the VoiceOver being at +0x3e8)
    bool vo_busy() const;
    void cameraSpot(int area, int spot, float out[4]) const;   // 0x4ec180 (0x4ec140 sets the camera there)
    char skip_key = 0x1b;                            // +0x74: Escape (0x51 'Q' when 0x5a7579 is set)
    bool start_in_london = false;                    // +0x1818 (Big Ben chosen in the sign-in room)
    bool returning = false;                          // +0x1819 (back in the sign-in room: "WENgreet3")
    int frame_color = 0;                             // 0x4ef7b0(n) / +0x788 (port: the colour record itself: TODO)
    bool key(int vk) { (void)vk; return false; }     // 0x4baef0: a folded no-op that returns false
};

State &state();                                      // *0x63171c

}  // namespace Game
