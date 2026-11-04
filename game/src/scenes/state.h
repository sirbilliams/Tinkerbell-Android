// The game's global state object (0x1840 bytes, created by My_Stage +0x10 0x4e72d0 -> ctor 0x4ec6e0, pointer at
// 0x63171c). Only the parts the scenes ported so far use are here; the rest comes with those scenes.
#pragma once
#include "../vision/database.h"
#include <string>
#include <vector>
#include <cstdint>
namespace Vision { class Graphic; class Ras; class Sound; }

namespace Game {

// One of an area's five puzzle pieces (TokenInfo, vtable 0x517bc8, 0x18 bytes, ctor 0x4ed1e0). The state (+4):
// 0 none, 1 hidden somewhere in the area (to be found), 2 / 3 given by the area's first / second game, 4 won
// (Skull Rock counts 5 and 6 instead).
struct TokenInfo {
    int state = 0;                                   // +0x04
    float x = 0, y = 0, z = 0;                       // +0x08..+0x10: where it hides
    bool easy = true;                                // +0x14: an easy hiding place (in the open)
    void setPos(float px, float py, float pz) { x = px; y = py; z = pz; }   // v1 0x4ed200
    void setHard() { easy = false; }                 // v2 0x4ed240
    void setEasy() { easy = true; }                  // v3 0x4ed250
};
// A saved piece (0x14 bytes): keys "location<n>", "active<n>", "x<n>", "y<n>", "z<n>" in "<Area> Tokens".
struct TokenRecord { int state = 0; bool easy = false; float x = 0, y = 0, z = 0; };

// The hiding places of an area (0x28 bytes, ctor 0x4f1110): "hiding_places.dat", the area's section, its "easy"
// and "hard" places; handed out at random, the first `skill` pieces to hard places.
struct HidingPlaces {
    struct P { float x, y, z; };
    void load(const char *area);                     // 0x4ecbf0
    void assign(TokenInfo *t, int skill);            // 0x4ecf90
    std::vector<P> easy;                             // +0x04
    std::vector<P> hard;                             // +0x14 (each new one put first)
    bool shuffled = false;                           // +0x20
    int given = 0;                                   // +0x24
};

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
    std::vector<TokenRecord> tokens(const char *section);   // 0x4e9e40 (empty when the section is)
    void setTokens(const char *section, const std::vector<TokenRecord> &v);   // 0x4ea1b0 (the first five)
    void setGameInfo(const char *game, int v);       // 0x4e9db0: the game's section, "Info"
    bool getDust(int *out) { return getInt("Dust Amount", out); }   // 0x4ea760
    void setDust(int v) { setInt("Dust Amount", v); }                // 0x4ea7b0
    int gameInfo(const char *game);                  // 0x4e9d70: the game's section, "Info" (0 when missing)
    // "<area> Treasure Map" / "Map Segment" present: 0x4ea870 Village, 0x4ea9f0 Starry, 0x4ea970 Lost,
    // 0x4ea8f0 Skull, 0x4ea7f0 Mermaid
    bool mapSegment(const char *area);
    bool anyMapSegment();
    void setMapSegment(const char *area);            // 0x4ea830.. (Lost: 0x4ea9b0): "<Area> Treasure Map" = "true"
    bool allMapSegments();                           // 0x4eaa70: Mermaid, Village, Skull, Lost, Starry                            // 0x4eabb0: Mermaid, Village, Skull, Lost, Starry
    bool hasXSpot(bool *out) { return flag(cur, "Has X Spot", out); }   // 0x4ea690
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
    int skill = 1;                                   // +0x68 (0x4ec920: 1)
    Vision::Sound *throwdust = nullptr;              // +0x7c "GLOB_FX_throwdust"
    bool dusting = false;                            // +0x3c4: the pixie dust is on the cursor
    int dust = 0;                                    // +0x3c8: the pixie dust in the bag (0..50)
    int last_game = 0, last_result = 0;             // +0x79c +0x7a0: the last mini game played and how it ended
    bool tutorial_skipped = false;                   // +0x7bc: Escape in the Lost Boys' tutorial
    int cur_spot[6] = {};                            // +0x1580 (+0x13a0 + 0x1e0): each area's camera spot
    void areaSpot(int area, float out[4]) const { cameraSpot(area, cur_spot[area], out); }   // 0x4ec0a0
    void setAreaSpot(int area, int spot) { cur_spot[area] = spot; }                          // 0x4ec120
    std::string last_scene = "lost";                 // +0x1830: the last 3D scene (not "treasure")
    // the areas' game records (the first int of each; 5 games per area): Indian village +0x15a0 (0x1c apart),
    // Starry skies +0x162c, Skull rock +0x16a4, Lost Boys +0x172c, Mermaid cove +0x17a4 (0x18 apart).
    // port: what fills them (the areas and the save) is not ported yet; 0 for a new player.
    int games_indv[5] = {}, games_star[5] = {}, games_skrl[5] = {}, games_mcov[5] = {};
    TokenInfo lost[5];                               // +0x1728 (states at +0x172c)
    void loadLostTokens();                           // 0x4ee860: from the save, or hidden and handed out anew
    void saveLostTokens();                           // 0x4eec40
    bool anyLost(int v) const { for (const TokenInfo &t : lost) if (t.state == v) return true; return false; }   // 0x4eef20 (5)
    bool allLost(int v) const { for (const TokenInfo &t : lost) if (t.state != v) return false; return true; }   // 0x4eef50 (4)
    bool hover_up = false, hover_down = false;       // +0x75 +0x76: the 3D eye rises to 200 / stays
    bool enter_released = true;                      // +0x77
    Saves saves;                                     // +0x3cc
    // +0x560: a voice line is playing (the byte is the VoiceOver member's +0x178 flag, the VoiceOver being at +0x3e8)
    bool vo_busy() const;
    void cameraSpot(int area, int spot, float out[4]) const;   // 0x4ec180 (0x4ec140 sets the camera there)
    char skip_key = 0x1b;                            // +0x74: Escape (0x51 'Q' when 0x5a7579 is set)
    bool start_in_london = false;                    // +0x1818 (Big Ben chosen in the sign-in room)
    bool returning = false;                          // +0x1819 (back in the sign-in room: "WENgreet3")
    int frame_color = 0;                             // 0x4ef7b0(n) (the colour itself: port: not needed)
    // +0x650: one record per sign-in slot (6 x 0x34, ctor 0x4c63c0): "first time" bytes, 0x00..0x18 set, the rest
    // clear, read and cleared (0x4c6c50..0x4c6d20) or read and set (0x4c6d30..) by the characters; +0x28 a line.
    struct Firsts {
        Firsts() { reset(); }
        void reset() { for (int i = 0; i < 0x28; i++) b[i] = i < 0x19; }   // 0x4c6450
        bool take(int i) { bool v = b[i]; b[i] = 0; return v; }           // 0x4c6c50.. (+0xa..+0x18)
        bool mark(int i) { bool v = b[i]; b[i] = 1; return v; }           // 0x4c6d30.. (+0x19..)
        uint8_t b[0x28];
        std::string line;                            // +0x28
    };
    Firsts players[6];                               // +0x650
    Firsts *firsts = &players[0];                    // +0x788: the signed-in player's (set at sign-in)
    bool key(int vk) { (void)vk; return false; }     // 0x4baef0: a folded no-op that returns false
};

State &state();                                      // *0x63171c

}  // namespace Game
