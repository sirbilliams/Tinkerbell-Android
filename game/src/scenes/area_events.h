// The tasks an area scene runs when a character is clicked or a mini game has ended, reconstructed from
// "Tinker Bell.exe" (shared by the five Neverland areas; their branches pick the area by the scene's name):
//   CharacterClick  vtable 0x517010, ctor 0x4c0990, dtor 0x43e270, start 0x4c12a0, stop 0x4c12c0, say 0x4c0b00,
//                   run v10 0x4c1100, notify2 v9 0x4c12f0 (Space stops it)
//   EndOfGame       vtable 0x517080, ctor 0x4c1820, dtor 0x43e050, setLines 0x4c1900, setBag 0x4c19d0,
//                   start 0x4c19e0, run v10 0x4c1a00, award 0x4c1d40, markPlayed 0x4c2230, nextLine 0x4c24b0
//   EndOfPuzzleGame vtable 0x5170b0, ctor 0x4c2950, dtor 0x43e100, setLines 0x4c2a10, setBag 0x4c2ac0,
//                   setPiece 0x4c2ad0, run v10 0x4c2ae0
//   SignClick       vtable 0x517274, ctor 0x4c7e10, dtor 0x43e190, set 0x4c7f00, start 0x4c7fa0, stop 0x4c7fe0,
//                   run v10 0x4c8080, notify2 v9 0x4c8720 (Space skips the sign's picture)
//   SceneInactivity vtable 0x517240, ctor 0x4c6e50, dtor 0x43e980, init 0x4c70e0, add 0x4c7100, setSpot 0x4c73f0,
//                   start 0x4c7730, stop 0x4c7770, run v10 0x4c7460, notify2 v9 0x4c7520, prompt 0x4c7540,
//                   allowed 0x4c77b0
// The game just played is the state's +0x79c (0 none, 1 the puzzle game, 2 / 3 the area's first / second game) and
// how it ended +0x7a0.
#pragma once
#include "../vision/engine.h"
#include "../raytrace/raytrace.h"
#include <string>
#include <vector>

namespace Game {

class PixieDustBag;
class MapPiece;

// An area's games, by scene name (the exe's string compares on the current scene's name).
struct AreaGames {
    const char *scene;                               // 0x631750.. ("Mermaid", "Indian_Village", "starry", "lost", "skull")
    const char *game2, *game3;                       // +0x79c == 2 / 3
    const char *puzzle;                              // the puzzle game (a map segment)
    const char *segment;                             // "<segment> Treasure Map"
};
const AreaGames *current_area();                     // the current scene's, or null

class CharacterClick : public Vision::Task {
public:
    // 0x4c0990: the exe takes ten lines and keeps only the 1st-3rd (twice), 7th (the key), 8th (twice) and 10th
    CharacterClick(const char *greet, const char *recrd, const char *fun, const char *, const char *,
                   const char *, const char *key, const char *more, const char *, const char *overly);
    ~CharacterClick() override {}                    // 0x43e270
    void start();                                    // 0x4c12a0
    void stop();                                     // 0x4c12c0
    void say(bool first);                            // 0x4c0b00: the line for the game's progress, played
    void run() override;                             // v10 0x4c1100
    void notify2(Vision::Service *s) override;       // v9 0x4c12f0
    std::string first[3];                            // +0xc0 +0xcc +0xd8: greet, record, fun (played before)
    std::string again[3];                            // +0xe4 +0xf0 +0xfc: the same (not played before)
    std::string key;                                 // +0x108: "BUCKET_TOSS_CHAR"...
    std::string more, more2;                         // +0x114 +0x120: "play more" (the same line twice)
    std::string overly;                              // +0x12c: said when "more" was
    std::string line;                                // +0x138: the line said
    Vision::Graphic *sign = nullptr;                 // +0x144: shown while the "overly" line plays
    Vision::Keyboard keys;                           // +0x148
    uint32_t started = 0;                            // +0x17c: set by the scene
};

class EndOfGame : public Vision::Task {
public:
    EndOfGame() : Task(nullptr) {}                   // 0x4c1820
    ~EndOfGame() override {}                         // 0x43e050
    // 0x4c1900: too bad, won (piece), won (big dust), won (small dust), "play the other game", "play the puzzle"
    void setLines(const char *a, const char *b, const char *c, const char *d, const char *e, const char *f);
    void setBag(PixieDustBag *b) { bag = b; }        // 0x4c19d0
    void start();                                    // 0x4c19e0
    void run() override;                             // v10 0x4c1a00
    void award();                                    // 0x4c1d40: the game's piece won
    void markPlayed();                               // 0x4c2230: "Info" 2
    void nextLine();                                 // 0x4c24b0: what to play next
    std::string lines[6];                            // +0xc0..+0xfc
    std::string next;                                // +0x108
    PixieDustBag *bag = nullptr;                     // +0x114
};

class EndOfPuzzleGame : public Vision::Task {
public:
    EndOfPuzzleGame() : Task(nullptr) {}             // 0x4c2950
    ~EndOfPuzzleGame() override {}                   // 0x43e100
    // 0x4c2a10: won (the segment), "the treasure map is not complete", too bad, won (big dust), won (small dust)
    void setLines(const char *a, const char *b, const char *c, const char *d, const char *e);
    void setBag(PixieDustBag *b) { bag = b; }        // 0x4c2ac0
    void setPiece(MapPiece *p) { piece = p; }        // 0x4c2ad0
    void start() { reset(); activate(); }            // (the scene's, as EndOfGame's 0x4c19e0)
    void run() override;                             // v10 0x4c2ae0
    std::string lines[5];                            // +0xc0..+0xf0
    MapPiece *piece = nullptr;                       // +0xfc
    PixieDustBag *bag = nullptr;                     // +0x100
};

// A game's sign clicked: the character tells about the game (the first time) or invites to play it, or says it
// cannot be played yet (a puzzle game before its area is done).
class SignClick : public Vision::Task {
public:
    SignClick();                                     // 0x4c7e10
    ~SignClick() override {}                         // 0x43e190
    // 0x4c7f00: the game, its "info" line, its "go" line, the "not yet" line, a picture shown after
    void set(const char *game, const char *info, const char *go, const char *nogo, Vision::Graphic *pic);
    void start();                                    // 0x4c7fa0 (only when not running)
    void stop();                                     // 0x4c7fe0
    void run() override;                             // v10 0x4c8080
    void notify2(Vision::Service *s) override;       // v9 0x4c8720
    std::string game, info, go, nogo;                // +0xc0 +0xcc +0xd8 +0xe4
    std::string line, line_z;                        // +0xf0 +0xfc: said (and its "_z" follow-up)
    Vision::Graphic *pic = nullptr;                  // +0x108
    bool show_pic = false;                           // +0x10c
    Vision::Keyboard keys;                           // +0x110
    static bool active_sign;                         // 0x6311ec
};

// After a while with no input (+0xc4 ms, the voice-over silent): the first of three spots close enough to the
// camera, its character prompts the player (its three lines in turn), if its game allows.
class SceneInactivity : public Vision::Task {
public:
    SceneInactivity();                               // 0x4c6e50
    ~SceneInactivity() override {}                   // 0x43e980
    void init(Raytrace::RayTrace *r, uint32_t ms) { rt = r; timeout = ms; }   // 0x4c70e0
    // 0x4c7100: spot i's reach and lines; the game key (appended in call order) must allow it ("" always)
    void add(int i, float reach, const char *l1, const char *l2, const char *l3, const char *key);
    void setSpot(int i, float x, float z) { xs[i] = x; zs[i] = z; }   // 0x4c73f0
    void start();                                    // 0x4c7730
    void stop();                                     // 0x4c7770
    void run() override;                             // v10 0x4c7460
    void notify2(Vision::Service *) override { last = Vision::frame_time(); }   // v9 0x4c7520
    void prompt();                                   // 0x4c7540
    bool allowed(int i);                             // 0x4c77b0
    Raytrace::RayTrace *rt = nullptr;                // +0xc0
    uint32_t timeout = 0;                            // +0xc4
    int next[3] = {};                                // +0xcc (vector of 3)
    float xs[3] = {}, zs[3] = {};                    // +0xdc +0xec (vectors of 3)
    float reach[3] = {};                             // +0xfc (vector of 3)
    std::vector<std::string> lines[3];               // +0x10c (vector of 3 vectors)
    std::vector<std::string> keys;                   // +0x11c
    uint32_t last = 0;                               // +0x128
    Vision::Mouse mouse;                             // +0x12c
    Vision::Keyboard keyboard;                       // +0x160
};

}  // namespace Game
