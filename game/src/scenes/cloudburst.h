// Cloud Burst, Foxy's puzzle game in Starry Skies (played in the Starry Skies scene itself; its object is the
// scene's +0x3fd8), reconstructed from "Tinker Bell.exe" (no RTTI: 0x1e90 bytes, ctor 0x478b10, dtor 0x478d80).
// The five animal pieces stand on a shelf by Foxy. Foxy asks a riddle ("which animal does the cat run from?",
// "which one is bigger than the mouse?", "which one purrs?"); the animal asked about walks in from a side, and the
// player clicks the answer on the shelf: it comes in too, and the two chase each other off, or meet and vanish in
// a puff. A wrong answer vanishes and the riddle comes round again. The fuse (from the second game on) is the clock.
//   questions 0x478ed0 (with add 0x47a290), hideAll 0x47a320, ask 0x47a380, showAnimal 0x47a490, praise 0x47a570,
//   wrong 0x47a650, huntRight 0x47a8f0, fleeRight 0x47ac30, sizeRight 0x47af60, anyWrong 0x47b300,
//   anyRight 0x47b510, end 0x47b740, update 0x47b7c0, choose 0x47bc10
//   the question pool (+0x1d14, no vtable): add 0x471a20, start 0x471be0, next 0x471e70, removeCurrent 0x472060,
//   total 0x472120, the shuffles 0x472380 / 0x457be0 (MSVC 6's random_shuffle)
#pragma once
#include "../raytrace/raytrace.h"
#include "../raytrace/mover.h"
#include "voacts.h"
#include <utility>
#include <vector>

namespace Game {

// A riddle (0x48 bytes at +0x9c, ctor 0x478b00).
struct CloudQuestion {
    int answers[5] = {};                             // +0x00: cat 0, dog 1, elephant 2, mouse 3, tiger 4
    int count = 0;                                   // +0x14
    char line[0x28] = {};                            // +0x18: Foxy's line
    // +0x40: 0 any animal (param -1), 1 who `param` hunts, 2 who `param` runs from, 3 bigger / smaller than it
    int kind = 0;
    int param = 0;                                   // +0x44: the animal asked about
};

// The riddles still to ask (+0x1d14): three groups (easy to hard), how many of each this game by skill, each
// riddle with how often it has been asked.
struct CloudQuestionPool {
    void add(int q, int group);                      // 0x471a20
    void start();                                    // 0x471be0: the counts by skill, the pools filled, shuffled
    int next();                                      // 0x471e70: a group still due at random, its riddle asked least
    void removeCurrent();                            // 0x472060: answered: out of its pool, one fewer due
    int total() const { return left[2] + left[1] + left[0]; }   // 0x472120
    std::vector<int> ids[3];                         // +0x00 +0x10 +0x20
    std::vector<std::pair<int, int>> pool[3];        // +0x30 +0x40 +0x50: (riddle, times asked)
    int left[3] = {};                                // +0x60 +0x64 +0x68
    int cur = -1;                                    // +0x6c (a pointer into pool[group]; port: its index)
    int group = 0;                                   // +0x70
};

class CloudBurstGame {
public:
    CloudBurstGame();                                // 0x478b10
    ~CloudBurstGame();                               // 0x478d80
    void questions();                                // 0x478ed0: the animals' sprites, the riddles
    CloudQuestion *addQuestion(const char *line, int kind, int param, int group);   // 0x47a290
    void hideAll();                                  // 0x47a320
    void ask();                                      // 0x47a380 (state 3)
    void showAnimal();                               // 0x47a490 (state 4)
    void praise();                                   // 0x47a570
    void wrong();                                    // 0x47a650 (state 8)
    void huntRight();                                // 0x47a8f0 (state 9)
    void fleeRight();                                // 0x47ac30 (state 10)
    void sizeRight();                                // 0x47af60 (state 11)
    void anyWrong();                                 // 0x47b300 (state 13)
    void anyRight();                                 // 0x47b510 (state 14)
    void end();                                      // 0x47b740
    void start() { pool.start(); }                   // (the scene's: +0x1d14's 0x471be0)
    void update();                                   // 0x47b7c0: every 30 ms while it is on
    void choose(int animal);                         // 0x47bc10: an animal on the shelf clicked

    bool busy = false;                               // +0x00: a riddle being said or an answer shown
    // +0x04 "<a>poself" (standing, from the left), +0x18 "<a>posert", +0x2c "<a>huntlf", +0x40 "<a>huntrt",
    // +0x54 "<a>fleelf", +0x68 "<a>fleert", +0x7c "<a>poof" (a = cat dog ele mou tig)
    Raytrace::Sprite3D *posel[5] = {}, *poser[5] = {}, *huntl[5] = {}, *huntr[5] = {}, *fleel[5] = {},
                       *fleer[5] = {}, *poof[5] = {};
    bool left_side = false;                          // +0x90: the animal asked about comes from the left
    float off = 0;                                   // +0x94: where it stops (from the middle)
    int nq = 0;                                      // +0x98
    CloudQuestion q[100];                            // +0x9c
    int cur = 0;                                     // +0x1cbc: the riddle
    int chosen = 0;                                  // +0x1cc0: the animal clicked
    bool right = false;                              // +0x1cc4
    int sub = 0;                                     // +0x1cc8: the answer's own steps
    // +0x1ccc: a random number object (0x425d10); its draws are the game's rand() (0x425d40)
    Raytrace::Sprite3D *a = nullptr, *b = nullptr;   // +0x1cd0 the animal asked about, +0x1cd4 the answer
    Raytrace::Sprite3D *c = nullptr, *d = nullptr;   // +0x1cd8 +0x1cdc: their puffs
    int counter = 0;                                 // +0x1ce0: the puff's frame
    float dist = 0;                                  // +0x1ce4
    int asked[5] = {};                               // +0x1ce8: Tink's hint (the answers 9999999: never shown)
    Raytrace::CameraMover mover;                     // +0x1cfc
    CloudQuestionPool pool;                          // +0x1d14
    Vision::QuestionRepeater repeater;               // +0x1d88
    int state = 0;                                   // +0x1e8c
};

}  // namespace Game
