// Echo Echo, reconstructed from "Tinker Bell.exe" (see echoecho.h).
#include "echoecho.h"
#include "skull.h"
#include "state.h"
#include "tink3d.h"
#include "cannon.h"
#include "voiceover.h"
#include "voacts.h"
#include "tb.h"
#include <stdlib.h>
#include <string.h>

namespace Game {

using namespace Vision;
using Raytrace::Sprite3D;
using Raytrace::Attachment;

static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }   // 0x5a6e30

// ---------------------------------------------------------------- EchoInstrument
// 0x48a920: the old sound stopped and deleted, a new one of that name opened and played.
static void play_fresh(Sound *&snd, const char *name) {
    if (snd) {
        if (snd->isPlaying()) snd->stop();           // 0x41fec0, 0x41fe70
        delete snd;                                  // v3(1)
    }
    snd = Gorilla::gnew<Sound>(name);                // 0x4243c0 (0x348), 0x420210
    snd->open();                                     // 0x41f9d0(0)
    snd->play();                                     // 0x41fd50
}

void EchoInstrument::play(const char *n) { play_fresh(snd, n); }

bool EchoInstrument::playing() { return snd ? snd->isPlaying() : false; }   // 0x48a9c0

EchoInstrument::~EchoInstrument() {                  // 0x48ab10
    delete snd;
    snd = nullptr;
}

static EchoInstrument *make_inst(const char *name, const char *anim, const char *idle, const char *fx) {
    EchoInstrument *i = new EchoInstrument();        // 0x4f34f3 (0x1c)
    i->name = name;
    i->anim = anim;
    i->idle = idle;
    i->fx = fx;
    return i;
}

EchoInstruments::EchoInstruments(Raytrace::RayTrace *r, EchoEcho *e) : rt(r), echo(e) {   // 0x48a9d0
    inst[0] = make_inst("echo drum", "echo_ga_puzdrum_", "echo_ga_puzdrumid_", "echo_fx_drum");
    inst[1] = make_inst("echo mara", "echo_ga_puzmara_", "echo_ga_puzmaraid_", "echo_fx_marac");
    inst[2] = make_inst("echo horn", "echo_ga_puzhorn_", "echo_ga_puzhornid_", "echo_fx_horn");
    inst[3] = make_inst("echo flute", "echo_ga_puzflte_", "echo_ga_puzflteid_", "echo_fx_flute");
    inst[4] = make_inst("echo coco", "echo_ga_puzcoco_", "echo_ga_puzcocoid_", "echo_fx_coconut");
}

// 0x48ab30: on the player's turn (no instrument sounding) the one under the mouse; it animates and the game asks
// for the hand, the others idle.
void EchoInstruments::update() {
    int hov = 6;
    if (enabled) {
        bool sounding = false;
        for (EchoInstrument *i : inst)
            if (i->snd && i->snd->isPlaying()) { sounding = true; break; }   // 0x41fec0
        if (!sounding) {
            Attachment *a = rt->hitAttached(shelf, g_mouse.x, g_mouse.y, false);   // 0x42ca00
            for (int k = 0; k < 5; k++)
                if (a == inst[k]->att) { hov = k; break; }
        }
    }
    echo->hand = false;                              // +0xec
    for (int k = 0; k < 5; k++) {
        EchoInstrument *i = inst[k];
        if (k == hov) {
            echo->hand = true;
            i->on = true;
            i->att->setPrefix(i->anim);              // 0x42e4f0
        } else {
            i->on = false;
            i->att->setPrefix(i->idle);
        }
    }
}

// ---------------------------------------------------------------- EchoPlayer
EchoPlayer::~EchoPlayer() {                          // 0x489e10
    queue.clear();
    delete snd;
    snd = nullptr;
}

void EchoPlayer::play(const char *n) { play_fresh(snd, n); }   // 0x48a920 (on the player: its head is the Sound)

// ---------------------------------------------------------------- EchoEcho
EchoEcho::EchoEcho(Raytrace::RayTrace *r) : Task(nullptr), rt(r) {   // 0x4896d0
    inactivity.add("ECHO_VO_RABprompt", false);      // 0x4c46e0
}

EchoEcho::~EchoEcho() {                              // 0x4897b0
    teardown();                                      // port: the exe's leave has done it already
}

Vision::Graphic *EchoEcho::cursor(Vision::Graphic *def) {   // 0x489960
    return hand ? Game::state().cursor_active : def; // +0x60
}

// 0x489980: the game on; the echoes and the round cleared, the instruments' bar hidden, every instrument shown;
// the round's length and time by the skill (played before: one longer and timed).
void EchoEcho::start() {
    Skull_Scene &sc = skull_scene();
    sc.echo_on = true;                               // 0x5df041
    rounds = 0;                                      // +0xdc
    instr->picked = -1;                              // +0x14
    player->queue.clear();                           // (popped one by one)
    seq.clear();
    sc.tokens->visible = false;                      // 0x5df050 +0x7c
    for (EchoInstrument *i : instr->inst) i->att->on = true;   // +0x17c
    State &st = Game::state();
    int skill;
    if (!st.saves.getSkill(&skill)) skill = 1;       // 0x4e97a0
    switch (skill) {
    case 1: secs = 0xb4; length = 1; break;
    case 2: secs = 0x96; length = 2; break;
    case 3: secs = 0x78; length = 3; break;
    default: break;
    }
    int info = st.saves.gameInfo("Echo Echo Game");  // 0x4e9d70 (*0x543518)
    if (info == 1 || info == 2) {
        length++;
        fuse->setTime(secs);                         // 0x4e3ae0
        fuse->visible = true;                        // +0x7c
        timed = true;                                // +0x4ac
    } else {
        timed = false;
    }
    st.last_game = 4;                                // +0x79c
    reset();                                         // 0x421110
    activate();                                      // v6
    inactivity.start();                              // +0x30
}

void EchoEcho::skip() {                              // 0x489bd0
    Skull_Scene &sc = skull_scene();
    sc.hotsOn(true);                                 // 0x489560
    sc.signsOn(true);                                // 0x485810
    state = 4;
}

// 0x489c00: the instruments on the shelf (idle, 10 ms a frame; shown only once the five are won), the echoes.
void EchoEcho::setup() {
    instr = new EchoInstruments(rt, this);           // 0x4f34f3 (0x28)
    instr->shelf = rt->findSprite("AA_ECHO_GA_puzshelf_01");   // 0x42dc70
    for (EchoInstrument *i : instr->inst) {
        instr->shelf->attach(i->name, i->idle, 100); // 0x42f390
        i->att = rt->findAttached(i->name);          // 0x42dd00
        i->att->on = false;
        i->att->period = 10;                         // +0x184
    }
    bool shown = Game::state().anySkull(6);          // 0x4ee7c0
    for (EchoInstrument *i : instr->inst) i->att->on = shown;
    player = new EchoPlayer();                       // 0x4f34f3 (0x48)
    fuse = &Game::cannon();                          // state +0x7c0
}

// 0x489d80 (the attachments stay on the shelf)
void EchoEcho::teardown() {
    delete player;                                   // 0x489e10
    player = nullptr;
    if (instr) {
        for (EchoInstrument *&i : instr->inst) {
            delete i;                                // 0x48ab10
            i = nullptr;
        }
        delete instr;
    }
    instr = nullptr;
}

// 0x489ed0: on the player's turn with nothing sounding, a click on an instrument picks it (its sound; the cursor
// hidden until the answer is taken).
void EchoEcho::mouse() {
    Skull_Scene &sc = skull_scene();
    sc.resetIdle();                                  // 0x4a9c90
    if (Game::state().vo_busy()) return;             // 0x4bcb50
    if (player->snd && player->snd->isPlaying()) return;
    for (EchoInstrument *i : instr->inst)
        if (i->playing()) return;
    if (!instr->enabled) return;
    for (EchoInstrument *i : instr->inst)
        if (i->playing()) return;
    if (!mouse_down()) return;
    Attachment *a = rt->hitAttached(instr->shelf, g_mouse.x, g_mouse.y, false);   // 0x42ca00
    for (int k = 0; k < 5; k++) {
        if (a != instr->inst[k]->att) continue;
        instr->picked = k;
        instr->inst[k]->play(instr->inst[k]->fx);    // 0x48a920
        hide_cursor();                               // 0x4119e0
        return;
    }
}

// 0x489fc0: a new round of 1..length echoes (no two alike in a row), the rabbit's "play one / two..." (for one
// only a third of the time), the echoes queued.
void EchoEcho::makeRound() {
    seq.clear();
    int n = vision_random(length) + 1;               // 0x425d40 (state +8)
    int e = vision_random(5);
    seq.push_back(e);                                // 0x430dd0
    for (int k = n; k > 1; k--) {
        e = vision_random(5);
        while (e == seq.back()) e = vision_random(5);
        seq.push_back(e);
    }
    const char *line = nullptr;
    switch (seq.size()) {
    case 1: line = "echo_vo_playone"; break;
    case 2: line = "echo_vo_playtwo"; break;
    case 3: line = "echo_vo_playthree"; break;
    case 4: line = "echo_vo_playfour"; break;
    default: break;                                  // (never: at most four)
    }
    if (seq.size() != 1 || vision_random(3) == 0)
        if (line) VoiceOver::get().play(line);       // 0x4cca20 on the rabbit
    for (size_t i = 0; i < seq.size(); i++) player->queue.push_back(seq.at(i));
    skull_scene().resetIdle();                       // 0x4a9c90
    state = 0;
}

// 0x48a1e0: the player's turn (the rabbit's prompt after 15 s). The pick taken: right, the echo answered (the last
// one: a "nice" now and then, the round counted, the seventh wins; the instruments idle); wrong, "not right" and
// a new round. Nothing picked: the cursor back.
void EchoEcho::check() {
    Skull_Scene &sc = skull_scene();
    instr->enabled = true;                           // +0x24
    if (sc.idleTime() > 15000) {                     // 0x4a9c70
        VoiceOver::get().play("ECHO_VO_RABprompt");
        sc.resetIdle();
    }
    if (getenv("TB_ECHO_AUTO") && instr->picked == -1 && !seq.empty())   // test aid (not in the game): the right one
        instr->picked = seq.front();
    int p = instr->picked;
    instr->picked = -1;
    if (getenv("TB_DEBUG_SKULL") && p != -1)
        tb_log("skull: echo answer %d (wanted %d, %d left, round %d)", p, seq.front(), (int)seq.size(), rounds);
    if (p == seq.front()) {
        seq.erase(seq.begin());
        if (!seq.empty()) return;
        switch (vision_random(6)) {
        case 0: VoiceOver::get().play("ECHO_VO_RABnice1"); break;
        case 1: VoiceOver::get().play("ECHO_VO_RABnice2"); break;
        case 2: VoiceOver::get().play("ECHO_VO_RABnice3"); break;
        default: break;
        }
        rounds++;
        state = rounds >= 7 ? 2 : 1;
        instr->enabled = false;
        for (EchoInstrument *i : instr->inst) {
            i->on = false;
            i->att->setPrefix(i->idle);
        }
        return;
    }
    if (p == -1) {
        show_cursor();                               // 0x4119d0
        return;
    }
    instr->enabled = false;
    for (EchoInstrument *i : instr->inst) {
        i->on = false;
        i->att->setPrefix(i->idle);
    }
    VoiceOver::get().play("echo_vo_notright");
    state = 1;
}

bool EchoEcho::busy() {
    if (Game::state().vo_busy()) return true;        // 0x4bcb50
    if (player->snd && player->snd->isPlaying()) return true;
    for (EchoInstrument *i : instr->inst)
        if (i->playing()) return true;
    return false;
}

// v10 0x48a370: the camera to the shelf, the fuse lit (timed), the cave's tune; then until won, out of time or
// skipped: with nothing sounding, the next echo; then the instruments and the round's step. At the end: the
// fuse let burn out, everything quiet, the area's tune, the result (won: the star, or the cap, or played), the
// scene back to normal and the end of the puzzle game.
void EchoEcho::run() {
    wait(1);
    Skull_Scene &sc = skull_scene();
    State &st = Game::state();
    sc.forceOff();                                   // v24
    mover.start(rt, 4633.62f, 0.1f, 4004.42f, 92.2539f, 1000);   // 0x4316f0 (0x4590ccf6, 0x457a46b8, 0x42b881ff)
    while (mover.step()) wait(0x1e);                 // 0x431890
    if (timed) fuse->restart();                      // 0x4e3b30
    sc.music(2);                                     // 0x4894c0
    state = 1;
    for (;;) {
        if (state == 2 || state == 3) break;
        if (timed && !fuse->timeLeft()) {            // 0x4e3b90
            state = 3;
            break;
        }
        wait(0x1e);
        if (!busy()) {
            if (!player->queue.empty() && !(player->snd && player->snd->isPlaying())) {
                int k = player->queue.front();
                player->queue.pop_front();           // 0x48abf0
                player->play(player->names[k]);
                vision_log("%s", player->names[k]);  // 0x4208c0, 0x5358a4
            }
            if (!busy()) {
                instr->update();                     // 0x48ab30
                if (state == 0) check();
                else if (state == 1) makeRound();
            }
        }
        if (state == 4) break;
    }
    if (state == 3 && fuse->visible && !fuse->timeLeft() && fuse->running())
        while (fuse->running()) wait(0x1e);          // 0x4e3b80
    sc.rabbit.rest();                                // 0x4cce30 (on +0x373c)
    if (player->snd) player->snd->stop();
    for (EchoInstrument *i : instr->inst)
        if (i->snd) i->snd->stop();
    instr->enabled = false;
    for (EchoInstrument *i : instr->inst) {
        i->on = false;
        i->att->setPrefix(i->idle);
    }
    instr->update();
    sc.music(1);
    show_cursor();                                   // 0x4119d0
    if (state == 2) {
        switch (st.saves.gameInfo("Echo Echo Game")) {
        case 0:
            sc.echo.star->on = true;                 // 0x5e00e8 (+0x5460)
            st.saves.setMapSegment("Skull");         // 0x4ea930
            st.last_result = 2;                      // +0x7a0
            break;
        case 1:
            sc.echo.cap->on = true;                  // 0x5e00e4 (+0x545c)
            st.last_result = 3;
            break;
        case 2:
            st.last_result = 4;
            break;
        default: break;
        }
    } else if (state == 3) {
        state = 4;
        if (st.saves.gameInfo("Echo Echo Game") != 0) st.last_result = 0;
    } else {
        st.last_game = 0;                            // +0x79c
    }
    wait(0);
    sc.echo_on = false;
    fuse->hide();                                    // 0x4e3bf0
    sc.forceNone();                                  // v23
    sc.echo_end.start();                             // 0x4c19e0 (+0x2fbc)
    sc.tink->wanders = true;                         // +0x13c
    deactivate();                                    // v7
    inactivity.stop();                               // +0x34
    wait(0);
}

// 0x48a8d0: the player's turn: Tink to the instrument the next echo wants (pointing at it); else home.
void EchoEcho::hint(Tinkerbell3D *tink) {
    Skull_Scene &sc = skull_scene();
    if (state != 0) {
        sc.tinkHome();                               // 0x4abb10
        return;
    }
    Attachment *a = instr->inst[seq.front()]->att;   // +0x18
    tink->clearQueue();                              // 0x4ddf30
    tink->toAttachment(a);                           // 0x4db660
    sc.tinkHome();
}

}  // namespace Game
