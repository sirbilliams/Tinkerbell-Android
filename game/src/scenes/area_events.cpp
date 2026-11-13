// CharacterClick, EndOfGame and EndOfPuzzleGame, reconstructed from "Tinker Bell.exe" (see area_events.h).
#include "area_events.h"
#include "state.h"
#include "voiceover.h"
#include "toolbar.h"
#include "mappiece.h"
#include <string.h>
#include <math.h>

namespace Game {

using Vision::VoiceOver;

void lost_refresh_tokens();                          // lost.cpp: 0x442e00 on the Lost Boys' scene (0x5ac4f8)

// The game names (0x5434fc..0x543534) and scene names (0x631700..0x631760).
static const AreaGames kAreas[5] = {
    {"Mermaid", "Coconuts Away Game", "Pearl Toss Game", "Shell Game", "Mermaid"},
    {"Indian_Village", "Bird Spy Game", "Flying Hoops Game", "Totem Trail Game", "Village"},
    {"starry", "Color The Cloud Game", "Backwards Go Game", "Cloud Burst Game", "Starry"},
    {"lost", "Mud Pie Game", "Pirate Flag Tag Game", "Bucket Toss Game", "Lost"},
    {"skull", "Firefly Game", "Shadow Boxing Game", "Echo Echo Game", "Skull"},
};

static bool scene_is(const char *n) {                // 0x5a7394's name == the global string (0x423cf0)
    Vision::Scene *s = Vision::current_scene();
    return s && strcmp(s->name(), n) == 0;
}

const AreaGames *current_area() {
    for (const AreaGames &a : kAreas)
        if (scene_is(a.scene)) return &a;
    return nullptr;
}

static VoiceOver &vo() { return VoiceOver::get(); }  // state +0x3e8

// ---------------------------------------------------------------- CharacterClick

CharacterClick::CharacterClick(const char *greet, const char *recrd, const char *fun, const char *, const char *,
                               const char *, const char *k, const char *m, const char *, const char *o)
    : Task(nullptr), key(k), more(m), more2(m), overly(o) {   // 0x4c0990
    first[0] = again[0] = greet;
    first[1] = again[1] = recrd;
    first[2] = again[2] = fun;
    keys.setClient(this);                            // 0x41f290
}

void CharacterClick::start() {                       // 0x4c12a0
    reset();                                         // 0x421110
    activate();                                      // +0x18
    keys.activate();
}

void CharacterClick::stop() {                        // 0x4c12c0
    if (sign) sign->visible = false;                 // +0x7c
    deactivate();                                    // +0x1c
    keys.deactivate();
}

// 0x4c12f0: Space ends it.
void CharacterClick::notify2(Vision::Service *s) {
    if (s != &keys || !Vision::g_key.down || Vision::g_key.vk != 0x20) return;   // 0x5a6dd8, 0x5a6d9c
    if (sign) sign->visible = false;
    deactivate();
    keys.deactivate();
}

// 0x4c0b00: the first time, the greeting. Later, by the character's game ("<Game>" "Info": 0 never played, 1
// won, 2 played): the greeting, the record or the fun line, the "first" set when the player's one-time flag for
// it is up (then cleared), the "again" set otherwise. A puzzle game's character offers "more" until its area is
// done (its pieces all won, or one over).
void CharacterClick::say(bool first_time) {
    State &st = Game::state();
    State::Firsts *f = st.firsts;                    // +0x788
    if (first_time) {
        line = first[0];
        vo().play(line.c_str());
        return;
    }
    // the simple games: the one-time flag (+0x788's byte), the game name
    struct G { const char *key; int flag; const char *game; };
    static const G simple[] = {
        {"COCONUTS_AWAY_CHAR", 0xa, "Coconuts Away Game"},     // 0x631198, 0x4c6c50
        {"PEARL_TOSS_CHAR", 0x14, "Pearl Toss Game"},          // 0x631158, 0x4c6c60
        {"BIRD_SPY_CHAR", 0xc, "Bird Spy Game"},               // 0x631148, 0x4c6c70
        {"COLOR_THE_CLOUD_CHAR", 0xd, "Color The Cloud Game"}, // 0x631128, 0x4c6c80
        {"BACKWARDS_GO_CHAR", 0xe, "Backwards Go Game"},       // 0x6310c8, 0x4c6c90
        {"SHADOW_BOXING_CHAR", 0xf, "Shadow Boxing Game"},     // 0x6311a8, 0x4c6ca0
        {"FIREFLY_CHAR", 0x10, "Firefly Game"},                // 0x6310e8, 0x4c6cb0
        {"MUD_PIE_CHAR", 0x11, "Mud Pie Game"},                // 0x631178, 0x4c6cc0
        {"FLYING_HOOPS_CHAR", 0x12, "Flying Hoops Game"},      // 0x631168, 0x4c6cd0
        {"PIRATE_FLAG_CHAR", 0x13, "Pirate Flag Tag Game"},    // 0x6310f8, 0x4c6ce0
    };
    bool fl = false;
    int info = -1;
    bool decided = false;                            // reached the info switch (code_r0x004c0b83)
    for (const G &g : simple) {
        if (key != g.key) continue;                  // 0x423cf0
        fl = f->take(g.flag);
        info = st.saves.gameInfo(g.game);            // 0x4e9d70
        decided = true;
        break;
    }
    if (!decided) {
        // the puzzle games: their area done goes to the info switch, else "more"
        bool done = false, known = true;
        if (key == "SHELL_CHAR") {                   // 0x631108 (the exe tests Pearl Toss's flag here)
            fl = f->take(0x14);
            info = st.saves.gameInfo("Shell Game");
            done = st.anyMermaid(5) || st.allMermaid(4);   // 0x4ef6b0 || 0x4ef6e0
        } else if (key == "ECHO_ECHO_CHAR") {        // 0x631188
            fl = f->take(0x15);
            info = st.saves.gameInfo("Echo Echo Game");
            int n5 = 0; bool any6 = false;
            for (int v : st.games_skrl) { if (v == 5) n5++; if (v == 6) any6 = true; }
            done = any6 || n5 == 5;                  // 0x4ee7c0 || 0x4ee7f0
        } else if (key == "CLOUD_BURST_CHAR") {      // 0x631138
            fl = f->take(0x16);
            info = st.saves.gameInfo("Cloud Burst Game");
            done = st.anyStarry(5) || st.allStarry(4);   // 0x4ee070 || 0x4ee0a0
        } else if (key == "BUCKET_TOSS_CHAR") {      // 0x6310d8
            fl = f->take(0x17);
            info = st.saves.gameInfo("Bucket Toss Game");
            done = st.anyLost(5) || st.allLost(4);   // 0x4eef20 || 0x4eef50
        } else if (key == "TOTEM_TRAIL_CHAR") {      // 0x631118
            fl = f->take(0x18);
            info = st.saves.gameInfo("Totem Trail Game");
            done = st.allVillage(4);                 // 0x4ed990
        } else {
            known = false;
        }
        if (!known) {                                // no key matched: the previous line again
            vo().play(line.c_str());
            return;
        }
        if (!done) {
            line = fl ? more : more2;                // +0x114 / +0x120
            vo().play(line.c_str());
            return;
        }
    }
    const std::string *set = fl ? first : again;
    if (info == 0) line = set[0];
    else if (info == 1) line = set[1];
    else if (info == 2) line = set[2];
    // other values: the line is left as it was
    vo().play(line.c_str());                         // 0x4c9400
}

// 0x4c1100: the line; after "more", the "overly" line with the sign shown, kept at least 5 s after `started`.
void CharacterClick::run() {
    wait(1);
    say(false);
    if (line == more) {                              // 0x423cf0 (+0x114)
        while (vo().isPlaying(line.c_str())) wait(0);
        vo().play(overly.c_str());
        sign->visible = true;                        // +0x144 +0x7c
        while (vo().isPlaying(overly.c_str()) || Vision::frame_time() < started + 5000) wait(0);
        sign->visible = false;
    }
    if (sign) sign->visible = false;
    deactivate();                                    // +0x1c
    keys.deactivate();
    finish();                                        // 0x420fe0
}

// ---------------------------------------------------------------- EndOfGame

void EndOfGame::setLines(const char *a, const char *b, const char *c, const char *d, const char *e, const char *f) {
    const char *v[6] = {a, b, c, d, e, f};           // 0x4c1900
    for (int i = 0; i < 6; i++) lines[i] = v[i] ? v[i] : "";
}

void EndOfGame::start() {                            // 0x4c19e0
    reset();
    activate();
}

// 0x4c1a00: after one of the area's two games (not the puzzle): lost or quit (1, 5) "too bad"; won (2) the
// piece and its line; a big (3) or small (4) win, the dust. Then, unless lost, the next suggestion.
void EndOfGame::run() {
    wait(1);
    State &st = Game::state();
    if (st.last_game == 0 || st.last_game == 1) {
        deactivate();
        finish();
        return;
    }
    switch (st.last_result) {
    case 1:
    case 5:
        vo().play(lines[0].c_str());
        while (vo().isPlaying(lines[0].c_str())) wait(0);
        break;
    case 2:
        award();
        vo().play(lines[1].c_str());
        while (st.vo_busy()) wait(0);                // +0x560
        break;
    case 3:
        markPlayed();
        vo().play(lines[2].c_str());
        while (st.vo_busy()) wait(0);
        if (bag) bag->refillLarge();                 // 0x4f05f0
        break;
    case 4:
        vo().play(lines[3].c_str());
        while (st.vo_busy()) wait(0);
        if (bag) bag->refillSmall();                 // 0x4f0570
        break;
    default:
        break;
    }
    if (st.last_result != 1) {
        nextLine();
        while (vo().isPlaying(next.c_str())) wait(0);
    }
    deactivate();
    finish();
}

// 0x4c1d40: the game "Info" 1 and its piece won (the area's records holding that game's piece become won);
// saved. port: the other areas' records are not saved yet and their scenes' pieces not refreshed (TODO with them).
void EndOfGame::award() {
    State &st = Game::state();
    const AreaGames *a = current_area();
    if (!a || (st.last_game != 2 && st.last_game != 3)) { st.saves.flush(); return; }
    bool g2 = st.last_game == 2;
    st.saves.setGameInfo(g2 ? a->game2 : a->game3, 1);   // 0x4e9db0
    auto mark = [](int *recs, int from, int to) {
        for (int i = 0; i < 5; i++)
            if (recs[i] == from) recs[i] = to;       // and the area's save each time (TODO)
    };
    if (!strcmp(a->scene, "Mermaid")) {
        int from = g2 ? 2 : 3;
        for (TokenInfo &t : st.mcov)
            if (t.state == from) {
                t.state = 4;
                st.saveMermaidTokens();              // 0x4ef3a0
            }
    }
    else if (!strcmp(a->scene, "Indian_Village")) {
        int from = g2 ? 1 : 2;
        for (VillageToken &t : st.village)
            if (t.state == from) {
                t.state = 4;
                st.saveVillageTokens();              // 0x4ed6b0
            }
    }
    else if (!strcmp(a->scene, "starry")) {
        int from = g2 ? 2 : 1;
        for (TokenInfo &t : st.starry)
            if (t.state == from) {
                t.state = 4;
                st.saveStarryTokens();               // 0x4edd90
            }
    }
    else if (!strcmp(a->scene, "lost")) {
        int from = g2 ? 3 : 2;
        for (TokenInfo &t : st.lost)
            if (t.state == from) {
                t.state = 4;
                st.saveLostTokens();                 // 0x4eec40
                lost_refresh_tokens();               // 0x442e00 (0x5ac4f8)
            }
    } else if (!strcmp(a->scene, "skull")) mark(st.games_skrl, g2 ? 2 : 3, 5);     // 0x4ee4e0, 0x5dac88
    st.saves.flush();                                // 0x4e9840
}

void EndOfGame::markPlayed() {                       // 0x4c2230
    State &st = Game::state();
    const AreaGames *a = current_area();
    if (a && (st.last_game == 2 || st.last_game == 3))
        st.saves.setGameInfo(st.last_game == 2 ? a->game2 : a->game3, 2);
    st.saves.flush();
}

// 0x4c24b0: the other game never played: suggest it; the area's pieces all won (or one over): suggest the
// puzzle game. Said at once.
void EndOfGame::nextLine() {
    State &st = Game::state();
    next = "";
    const AreaGames *a = current_area();
    if (a) {
        int info = 1;
        if (st.last_game == 2) info = st.saves.gameInfo(a->game3);
        else if (st.last_game == 3) info = st.saves.gameInfo(a->game2);
        bool done;
        if (!strcmp(a->scene, "Mermaid")) {
            done = st.allMermaid(4) || st.anyMermaid(5);   // 0x4ef6e0 || 0x4ef6b0
        } else if (!strcmp(a->scene, "Indian_Village")) {
            done = st.allVillage(4);                 // 0x4ed990
        } else if (!strcmp(a->scene, "starry")) {
            done = st.allStarry(4) || st.anyStarry(5);   // 0x4ee0a0 || 0x4ee070
        } else if (!strcmp(a->scene, "lost")) {
            done = st.allLost(4) || st.anyLost(5);   // 0x4eef50 || 0x4eef20
        } else {
            int n5 = 0; bool any6 = false;
            for (int v : st.games_skrl) { if (v == 5) n5++; if (v == 6) any6 = true; }
            done = n5 == 5 || any6;                  // 0x4ee7f0 || 0x4ee7c0
        }
        if (info == 0) next = lines[4];
        else if (done) next = lines[5];
    }
    vo().play(next.c_str());                         // 0x4c9400
}

// ---------------------------------------------------------------- EndOfPuzzleGame

void EndOfPuzzleGame::setLines(const char *a, const char *b, const char *c, const char *d, const char *e) {
    const char *v[5] = {a, b, c, d, e};              // 0x4c2a10
    for (int i = 0; i < 5; i++) lines[i] = v[i] ? v[i] : "";
}

// 0x4c2ae0: after the puzzle game (+0x79c != 0): won (2) the area's treasure map segment (the piece flies in);
// a big (3) or small (4) win, dust; lost (0) "too bad". When the treasure map is still missing segments, a line
// about it.
void EndOfPuzzleGame::run() {
    wait(1);
    State &st = Game::state();
    const AreaGames *a = current_area();
    if (st.last_game != 0) {
        switch (st.last_result) {
        case 2:
            if (a) st.saves.setMapSegment(a->segment);   // 0x4ea830 / 0x4ea930 / 0x4ea9b0 / 0x4eaa30 / 0x4ea8b0
            if (a) st.saves.setGameInfo(a->puzzle, 1);
            vo().play(lines[0].c_str());
            while (vo().isPlaying(lines[0].c_str())) wait(0);
            if (a) {
                if (!strcmp(a->scene, "skull")) vo().play("ECHO_VO_RABshadow");
                piece->show();                       // v13 +0x34
                while (piece->busy()) wait(0);       // v15 +0x3c
                if (!strcmp(a->scene, "skull"))
                    while (vo().isPlaying("ECHO_VO_RABshadow")) wait(0);
                st.saves.setMapSegment(a->segment);
            }
            st.saves.flush();
            if (!st.saves.allMapSegments()) {        // 0x4eaa70
                vo().play(lines[1].c_str());
                while (vo().isPlaying(lines[1].c_str())) wait(0);
            }
            break;
        case 0:
            vo().play(lines[2].c_str());
            while (vo().isPlaying(lines[2].c_str())) wait(0);
            break;
        case 3:
            if (a) st.saves.setGameInfo(a->puzzle, 2);
            st.saves.flush();
            vo().play(lines[3].c_str());
            while (vo().isPlaying(lines[3].c_str())) wait(0);
            if (!st.saves.allMapSegments()) {
                vo().play(lines[1].c_str());
                while (vo().isPlaying(lines[1].c_str())) wait(0);
            }
            if (bag) bag->refillLarge();
            break;
        case 4:
            vo().play(lines[4].c_str());
            while (st.vo_busy()) wait(0);
            if (!st.saves.allMapSegments()) {
                vo().play(lines[1].c_str());
                while (vo().isPlaying(lines[1].c_str())) wait(0);
            }
            if (bag) bag->refillSmall();
            break;
        default:                                     // 1 and others: nothing
            break;
        }
    }
    deactivate();
    finish();
}

}  // namespace Game

namespace Game {

// ---------------------------------------------------------------- SignClick

bool SignClick::active_sign = false;                 // 0x6311ec

SignClick::SignClick() : Task(nullptr) { keys.setClient(this); }   // 0x4c7e10

void SignClick::set(const char *g, const char *i, const char *go_, const char *n, Vision::Graphic *p) {   // 0x4c7f00
    game = g ? g : "";
    info = i ? i : "";
    go = go_ ? go_ : "";
    nogo = n ? n : "";
    pic = p;
}

void SignClick::start() {                            // 0x4c7fa0
    if (active()) return;                            // +0x14
    reset();
    activate();
    keys.activate();
    active_sign = true;
}

void SignClick::stop() {                             // 0x4c7fe0
    deactivate();
    if (vo().isPlaying(line.c_str()) || vo().isPlaying(line_z.c_str())) vo().stop();   // 0x4c9260
    if (pic) pic->visible = false;
    keys.deactivate();
    active_sign = false;
}

// 0x4c8720: Space, while the picture is to be shown: everything stops.
void SignClick::notify2(Vision::Service *s) {
    if (s != &keys || !Vision::g_key.down || Vision::g_key.vk != 0x20) return;
    if (!pic || !show_pic) return;
    deactivate();
    if (vo().isPlaying(line.c_str()) || vo().isPlaying(line_z.c_str())) vo().stop();
    if (pic) pic->visible = false;
    keys.deactivate();
    active_sign = false;
}

// 0x4c8080: the line for the game: a puzzle game before its area is done "not yet"; otherwise the "info" line the
// first time (the player's one-time byte, set now) and the "go" line after. Flying Hoops' first time shows the
// picture after its line (3 s). A line ending in "_y" goes on into its "_z".
void SignClick::run() {
    wait(1);
    State &st = Game::state();
    State::Firsts *f = st.firsts;                    // +0x788
    show_pic = false;
    struct G { const char *game; int flag; int area; };   // area: puzzle check (0 none)
    static const G gs[] = {
        {"Shell Game", 0x19, 1}, {"Pearl Toss Game", 0x1a, 0}, {"Coconuts Away Game", 0x1b, 0},
        {"Echo Echo Game", 0x1c, 2}, {"Shadow Boxing Game", 0x1d, 0}, {"Firefly Game", 0x1e, 0},
        {"Bucket Toss Game", 0x1f, 3}, {"Pirate Flag Tag Game", 0x20, 0}, {"Mud Pie Game", 0x21, 0},
        {"Cloud Burst Game", 0x22, 4}, {"Backwards Go Game", 0x23, 0}, {"Color The Cloud Game", 0x24, 0},
        {"Totem Trail Game", 0x25, 5}, {"Flying Hoops Game", 0x26, 0}, {"Bird Spy Game", 0x27, 0},
    };
    for (const G &g : gs) {
        if (game != g.game) continue;                // 0x423c60
        bool done = true;
        auto n_of = [](const int *a, int v) { int n = 0; for (int i = 0; i < 5; i++) if (a[i] == v) n++; return n; };
        switch (g.area) {
        case 1: done = st.anyMermaid(5) || st.allMermaid(4); break;   // 0x4ef6b0 / 0x4ef6e0
        case 2: done = n_of(st.games_skrl, 6) > 0 || n_of(st.games_skrl, 5) == 5; break;   // 0x4ee7c0 / 0x4ee7f0
        case 3: done = st.anyLost(5) || st.allLost(4); break;                             // 0x4eef20 / 0x4eef50
        case 4: done = st.anyStarry(5) || st.allStarry(4); break;                         // 0x4ee070 / 0x4ee0a0
        case 5: done = st.allVillage(4); break;                                            // 0x4ed990
        default: break;
        }
        if (!done) { line = nogo; break; }
        bool seen = f->mark(g.flag);                 // 0x4c6d30.. (read, then set)
        if (g.flag == 0x26 && !seen) {               // Flying Hoops: the info line, then the picture
            line = info;
            show_pic = true;
            break;
        }
        line = seen ? go : info;
        break;
    }
    line_z = line;                                   // +0xfc
    if (line_z.find("_y") != std::string::npos || line_z.find("_Y") != std::string::npos)   // 0x542634 / 0x542630
        line_z[line_z.size() - 1] = 'z';
    vo().play(line.c_str());
    for (;;) {
        if (!vo().isPlaying(line.c_str()) && !vo().isPlaying(line_z.c_str())) {
            if (pic && show_pic) {
                pic->visible = true;
                wait(3000);
                pic->visible = false;
            }
            deactivate();
            if (vo().isPlaying(line.c_str()) || vo().isPlaying(line_z.c_str())) vo().stop();
            if (pic) pic->visible = false;
            keys.deactivate();
            active_sign = false;
            finish();
            return;
        }
        wait(0);
    }
}

// ---------------------------------------------------------------- SceneInactivity

SceneInactivity::SceneInactivity() : Task(nullptr) {   // 0x4c6e50
    mouse.setClient(this);
    keyboard.setClient(this);
}

void SceneInactivity::add(int i, float r, const char *l1, const char *l2, const char *l3, const char *key) {
    reach[i] = r;                                    // 0x4c7100
    lines[i] = {l1, l2, l3};
    keys.push_back(key ? key : "");                  // 0x4c7b10 (appended)
}

void SceneInactivity::start() {                      // 0x4c7730
    last = Vision::frame_time();
    mouse.activate();
    keyboard.activate();
    reset();
    activate();
}

void SceneInactivity::stop() {                       // 0x4c7770
    if (!active()) return;
    mouse.deactivate();
    keyboard.deactivate();
    deactivate();
}

void SceneInactivity::run() {                        // 0x4c7460
    wait(1);
    for (;;) {
        wait(0);
        if (!Game::state().vo_busy()) {              // +0x560
            if (Vision::frame_time() - last > timeout) prompt();
        } else {
            last = Vision::frame_time();
        }
    }
}

// 0x4c77b0: "" always; a puzzle game's character only once its area is done.
bool SceneInactivity::allowed(int i) {
    const std::string &k = keys.at(i);               // (out of range: the exe's exception too)
    State &st = Game::state();
    auto n_of = [](const int *a, int v) { int n = 0; for (int j = 0; j < 5; j++) if (a[j] == v) n++; return n; };
    if (k == "") return true;
    if (k == "Bucket Toss Game") return st.allLost(4) || st.anyLost(5);
    if (k == "Shell Game") return st.allMermaid(4) || st.anyMermaid(5);
    if (k == "Totem Trail Game") return st.allVillage(4);
    if (k == "Cloud Burst Game") return st.allStarry(4) || st.anyStarry(5);
    if (k == "Echo Echo Game") return n_of(st.games_skrl, 5) == 5 || n_of(st.games_skrl, 6) > 0;
    return false;
}

// 0x4c7540: the first spot within its reach of the camera (0x42e400: the floor distance) prompts.
void SceneInactivity::prompt() {
    int i = 0;
    for (;; i++) {
        if (i > 2) { last = Vision::frame_time(); return; }
        double dx = (double)xs[i] - rt->camx, dz = (double)zs[i] - rt->camz;
        if (sqrt(dx * dx + dz * dz) < (double)reach[i]) break;
    }
    if (allowed(i)) {
        int n = next[i]++;
        vo().play(lines[i].at((size_t)n).c_str());   // 0x4c9400
    }
    if (next[i] == 3) next[i] = 0;
    last = Vision::frame_time();
}

}  // namespace Game
