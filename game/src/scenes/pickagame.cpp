// The pick-a-game room, reconstructed from "Tinker Bell.exe" (see pickagame.h).
#include "pickagame.h"
#include "state.h"
#include "voacts.h"
#include "voiceover.h"
#include "tb.h"
#include <stdlib.h>

namespace Game {

using namespace Vision;

static Sound &butclick() {                           // 0x62a8d0 (a static Sound, made at start-up)
    static Sound *s = new Sound("GLOB_FX_butclick");
    return *s;
}

// The fifteen games (0x4ae270..0x4ae5f0): their buttons, Peter's line, the area scene, the area and the game's spot
static const GameDesc k_back = {"GLOB_BU_BACKpgidl_01", "GLOB_BU_BACKpgrov_01", "GLOB_VO_PETgsback", "starry", 2, 1};
static const GameDesc k_bird = {"GLOB_BU_BIRDpgidl_01", "GLOB_BU_BIRDpgrov_01", "GLOB_VO_PETgsbird", "indian_village", 1, 3};
static const GameDesc k_buck = {"GLOB_BU_BUCKpgidl_01", "GLOB_BU_BUCKpgrov_01", "GLOB_VO_PETgsbuck", "lost", 3, 2};
static const GameDesc k_clou = {"GLOB_BU_CLOUpgidl_01", "GLOB_BU_CLOUpgrov_01", "GLOB_VO_PETgsclou", "starry", 2, 2};
static const GameDesc k_coco = {"GLOB_BU_COCOpgidl_01", "GLOB_BU_COCOpgrov_01", "GLOB_VO_PETgscoco", "mermaid", 5, 3};
static const GameDesc k_colr = {"GLOB_BU_COLRpgidl_01", "GLOB_BU_COLRpgrov_01", "GLOB_VO_PETgscolr", "starry", 2, 3};
static const GameDesc k_dcbl = {"GLOB_BU_DCBLpgidl_01", "GLOB_BU_DCBLpgrov_01", "GLOB_VO_PETgsdcbl", "skull", 4, 3};
static const GameDesc k_echo = {"GLOB_BU_ECHOpgidl_01", "GLOB_BU_ECHOpgrov_01", "GLOB_VO_PETgsecho", "skull", 4, 2};
static const GameDesc k_flyh = {"GLOB_BU_FLYHpgidl_01", "GLOB_BU_FLYHpgrov_01", "GLOB_VO_PETgsflyh", "indian_village", 1, 1};
static const GameDesc k_mudp = {"GLOB_BU_MUDPpgidl_01", "GLOB_BU_MUDPpgrov_01", "GLOB_VO_PETgsmudp", "lost", 3, 3};
static const GameDesc k_perl = {"GLOB_BU_PERLpgidl_01", "GLOB_BU_PERLpgrov_01", "GLOB_VO_PETgsperl", "mermaid", 5, 1};
static const GameDesc k_pirt = {"GLOB_BU_PIRTpgidl_01", "GLOB_BU_PIRTpgrov_01", "GLOB_VO_PETgspirt", "lost", 3, 1};
static const GameDesc k_shbx = {"GLOB_BU_SHBXpgidl_01", "GLOB_BU_SHBXpgrov_01", "GLOB_VO_PETgsshbx", "skull", 4, 1};
static const GameDesc k_shel = {"GLOB_BU_SHELpgidl_01", "GLOB_BU_SHELpgrov_01", "GLOB_VO_PETgsshel", "mermaid", 5, 2};
static const GameDesc k_totm = {"GLOB_BU_TOTMpgidl_01", "GLOB_BU_TOTMpgrov_01", "GLOB_VO_PETgsindv", "indian_village", 1, 2};

// ---------------------------------------------------------------- GameButton
GameButton::GameButton(const GameDesc &d)            // 0x4af450
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      idle(d.idle, 0, 0, 0, 0, 0, 0, true), rov(d.rov, 0, 0, 0, 0, 0, 0, true),
      scene(d.scene), area(d.area), game(d.game) {
    pause.set(this, d.line, 1000);                   // 0x3e8
}

GameButton::~GameButton() {}                         // 0x4af580

void GameButton::load() {                            // v15 0x4af650
    idle.load();
    idle.visible = true;                             // +0x114
    idle.setHit(1, true);                            // port: the exe's second word is whatever was on the stack
    rov.load();
    rov.visible = false;                             // +0x1bc
    if (!opened) {
        opened = true;
        butclick().open();
    }
    pause.start();                                   // +0x30
}

void GameButton::unload() {                          // v16 0x4af6c0
    Graphic::unload();
    pause.stop();                                    // +0x34
    idle.unload();
    rov.unload();
    if (opened) {
        opened = false;
        butclick().close();
    }
}

bool GameButton::hit(int x, int y) { return idle.hit(x, y); }   // v11 0x4afa60

void GameButton::draw() {                            // v14 0x4af720
    if (!visible) return;
    if (lit) rov.draw();
    else idle.draw();
}

// ---------------------------------------------------------------- ReturnButton
ReturnButton::ReturnButton()                         // 0x4af800
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      img("GLOB_BU_return_01", 0, 0, 0, 0, 0, 0, true), click("GLOB_FX_butclick") {
    img.load();
    img.setHit(-1, false);                           // (its box; hit while hidden)
    img.visible = false;                             // +0x114: shown when lit
    click.open();
    pause.set(this, "GLOB_VO_PETmap", 1000);
    pause.start();
}

ReturnButton::~ReturnButton() {                      // 0x4af940
    img.unload();
    click.close();
    pause.stop();
}

bool ReturnButton::hit(int x, int y) { return img.hit(x, y); }   // v11 0x4afa60

void ReturnButton::draw() {                          // v14 0x4afa40
    if (img.visible) img.draw();
}

// ---------------------------------------------------------------- PickAGameCursor
PickAGameCursor::PickAGameCursor() : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {}

void PickAGameCursor::draw() {                       // v14 0x4af7d0
    if (!cur) return;
    cur->x = x;
    cur->y = y;
    cur->draw();
}

// ---------------------------------------------------------------- PickAGame_Scene
PickAGame_Scene::PickAGame_Scene()                   // 0x4ae6a0
    : Scene("pickagame"),
      screen(0, 0, 0, 0, 0, true, true),
      bg("GLOB_BG_pickgame_01", 0, 0, 0, 0, 0, 0, true) {   // *0x540244
    butclick();
}

PickAGame_Scene::~PickAGame_Scene() {}               // 0x4ae7f0

void PickAGame_Scene::load2() {                      // v18 0x4ae950
    mouse.setClient(this);
    keyboard.setClient(this);
}

void PickAGame_Scene::notify2(Service *s) {          // v9 0x4ae900
    if (s == &mouse) onMouse();
    if (s == &keyboard && g_key.down) Game::state().key((int)g_key.vk);   // 0x4baef0
}

// v19 0x4ae970: the room, its fifteen buttons, the return button, the cursor, the music, Peter's line.
void PickAGame_Scene::enter() {
    pack = ras_open(name(), nullptr, &data_device());   // "pickagame"
    pack->load();
    ras_set_current(pack);
    mouse.activate();
    keyboard.activate();
    screen.activate();
    screen.loadAll();
    bg.load();
    screen.layer(0)->add(&bg);
    const GameDesc *all[15] = {&k_back, &k_bird, &k_buck, &k_clou, &k_coco, &k_colr, &k_dcbl, &k_echo,
                               &k_flyh, &k_mudp, &k_perl, &k_pirt, &k_shbx, &k_shel, &k_totm};
    for (const GameDesc *d : all) buttons.push_back(Gorilla::gnew<GameButton>(*d));   // 0x4243c0(0x310)
    for (GameButton *b : buttons) {
        b->load();
        screen.layer(0)->add(b);
    }
    ret = Gorilla::gnew<ReturnButton>();             // 0x4243c0(0x598)
    screen.layer(0)->add(ret);
    cursor.cur = Game::state().cursor;               // +0x310 = state +0x5c
    set_cursor(&cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
    show_cursor();
    music = new Sound("nvmp_mu_music");              // 0x4243c0(0x348)
    music->mode = 2;                                 // +0x338
    music->type = 6;                                 // +0x340
    music->open();
    music->play();
    activate();
    VoiceOver::get().play("GLOB_VO_PETpick");        // 0x4c9400
}

// 0x4af280: the button under the mouse lit (the hand cursor); clicked, the click sound and off to its area, set
// to that game; the return button to the map.
void PickAGame_Scene::onMouse() {
    if (getenv("TB_DEBUG_PICK")) tb_log("pick: mouse %d,%d down %d ret hit %d", g_mouse.x, g_mouse.y, g_mouse.ldown, ret->hit(g_mouse.x, g_mouse.y));
    State &st = Game::state();
    cursor.cur = st.cursor;                          // +0x5c
    bool down = g_mouse.ldown || g_mouse.rdown;      // 0x5a6e30
    for (GameButton *b : buttons) {
        if (!b->hit(g_mouse.x, g_mouse.y)) {
            b->lit = false;
            continue;
        }
        b->lit = true;
        if (getenv("TB_DEBUG_PICK")) tb_log("pick: over %s (%s, area %d game %d)", b->idle.name(), b->scene, b->area, b->game);
        if (down) butclick().play();
        cursor.cur = st.cursor_active;               // +0x60
        if (down) {
            st.setAreaSpot(b->area, b->game);        // 0x4ec120 (+0x13a0)
            change_scene(b->scene);
        }
    }
    if (ret->hit(g_mouse.x, g_mouse.y)) {
        ret->img.visible = true;
        if (down) ret->click.play();
        cursor.cur = st.cursor_active;
        if (down) change_scene("map");
        return;
    }
    ret->img.visible = false;
}

// v20 0x4af070
void PickAGame_Scene::leave() {
    if (music) music->close();
    delete music;
    music = nullptr;
    VoiceOver::get().stop();
    set_cursor(nullptr, g_cursor_graphic ? g_cursor_graphic->visible : false);
    deactivate();                                    // v7
    bg.detach();
    for (GameButton *b : buttons) {
        if (!b) continue;
        b->detach();
        b->unload();
        delete b;
    }
    buttons.clear();
    delete ret;
    ret = nullptr;
    cursor.unload();
    screen.unloadAll();
    mouse.deactivate();
    keyboard.deactivate();
    screen.deactivate();
    ras_release(pack);
    pack = nullptr;
    set_cursor(Game::state().cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
}

PickAGame_Scene &pickagame_scene() {                 // the static at 0x62a5b0
    static PickAGame_Scene s;
    return s;
}

}  // namespace Game
