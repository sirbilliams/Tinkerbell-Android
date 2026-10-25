// The sign-in room, reconstructed from "Tinker Bell.exe" (see signin.h for the classes and addresses).
#include "signin.h"
#include "state.h"
#include "tb.h"
#include <stdio.h>

namespace Game {

// ---------------------------------------------------------------- the cursor globals (0x4119b0 / 0x4119d0 / 0x4119e0)
static void set_cursor(Graphic *g, bool visible) {   // 0x4119b0
    g_cursor_graphic = g;
    if (g) g->visible = visible;
}
static void show_cursor() { g_cursor_graphic->visible = true; }                      // 0x4119d0
static void hide_cursor() { if (g_cursor_graphic) g_cursor_graphic->visible = false; }   // 0x4119e0

// 0x411e70: a graphic's depth; a graphic already in a layer is re-sorted.
static void set_depth(Graphic *g, int z) {
    g->z = z;
    if (g->layer) g->layer->sort();
}

// The button-down flag of the current mouse event (0x5a6e30, set from 0x418620's fourth argument for either button).
// port: the engine splits it into ldown / rdown.
static bool mouse_down() { return g_mouse.ldown || g_mouse.rdown; }

// ---------------------------------------------------------------- SignInButton
SignInButton::SignInButton(const char *i, const char *r, const char *s, const char *roll, const char *clk)   // 0x4afe70
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      idle(i, 0, 0, 0, 0, 0, 0, true), rov(r, 0, 0, 0, 0, 0, 0, true), sel(s, 0, 0, 0, 0, 0, 0, true) {
    if (roll) pause.set(this, roll, 1000);           // 0x4c5970: with no line, no owner and 3000 ms
    else pause.set(nullptr, nullptr, 3000);
    if (clk) click = new Sound(clk);                 // 0x420210
    idle.load();
    idle.visible = true;
    idle.setHit(1, true);
    rov.load();
    rov.visible = false;
    sel.load();
    sel.visible = false;
    if (pause.owner) pause.start();
    if (click) click->open();                        // 0x41f9d0(0)
}

SignInButton::~SignInButton() {                      // 0x4b0070
    pause.stop();
    if (click) { click->close(); delete click; }
    click = nullptr;
    idle.unload(); rov.unload(); sel.unload();
}

bool SignInButton::hit(int x, int y) {               // 0x4b0320
    if (enabled && visible) return idle.hit(x, y);
    return false;
}

// 0x4b0260: the three images as they are; while blinking (from `blink` for 3 s) the rollover and the idle image
// take turns every 400 ms.
void SignInButton::draw() {
    if (blink == 0 || frame_time() < blink) {
        if (idle.visible) idle.draw();
        if (rov.visible) rov.draw();
        if (sel.visible) sel.draw();
        return;
    }
    uint32_t t = frame_time() - blink;
    if (t % 800 < 400) rov.draw();
    else idle.draw();
    if (t < 3001) return;
    blink = 0;
}

// 0x4b01b0: the rollover image follows the mouse; a press on the button marks it clicked. The click sound only
// plays when the press comes with the first event over the button (the exe tests +0x3a5 before setting it).
bool SignInButton::track() {
    rov.visible = false;
    if (enabled && idle.visible) {
        clicked = false;
        if (idle.hit(g_mouse.x, g_mouse.y)) {
            rov.visible = true;
            if (mouse_down()) {
                if (click && !click->isPlaying() && !was_over) click->play();
                clicked = true;
            }
            was_over = true;
            return true;
        }
        was_over = false;
    }
    return false;
}

// ---------------------------------------------------------------- SignInFrame
SignInFrame::SignInFrame(const char *t, const char *i, const char *r, const char *s, const char *b, const char *c)
    : SignInButton(i, r, s, nullptr, nullptr),       // 0x4b0360
      bell(b, 0, 0, 0, 0, 0, 0, true), cap(c, 0, 0, 0, 0, 0, 0, true), tar(t, 0, 0, 0, 0, 0, 0, true) {
    bell.load(); bell.visible = false;
    cap.load(); cap.visible = false;
    tar.load(); tar.visible = false;
}

static void drop_pic(Image *&p) {
    if (!p) return;
    p->detach();                                     // 0x412200
    p->unload();
    delete p;
    p = nullptr;
}

SignInFrame::~SignInFrame() {                        // 0x4b0490
    bell.unload(); cap.unload(); tar.unload();
    delete location_pic; location_pic = nullptr;
    delete character_pic; character_pic = nullptr;
}

void SignInFrame::draw() {                           // 0x4b07b0
    SignInButton::draw();
    if (location_pic) location_pic->draw();
    if (character_pic) character_pic->draw();
    if (bell.visible) bell.draw();
    if (cap.visible) cap.draw();
}

// 0x4b0570 / 0x4b0810: the chosen picture, placed where the frame's target image is.
static Image *frame_pic(int n, const char *const names[6]) {
    if (n < 0 || n > 5) return nullptr;
    return new Image(names[n], 0, 0, 0, 0, 0, 0, true);
}
void SignInFrame::setLocation(int n) {
    static const char *const names[6] = { "SIGN_IF_LONDfrm_01", "SIGN_IF_SKLfrm_01", "SIGN_IF_INDVfrm_01",
                                          "SIGN_IF_MCOVfrm_01", "SIGN_IF_STARfrm_01", "SIGN_IF_LOSTfrm_01" };
    location = n;
    drop_pic(location_pic);
    location_pic = frame_pic(n, names);
    if (!location_pic) return;                       // port: the exe would call load on a null pointer here
    location_pic->load();
    location_pic->x = tar.x;
    location_pic->y = tar.y;
}
void SignInFrame::setCharacter(int n) {
    static const char *const names[6] = { "SIGN_IF_HOKfrm_01", "SIGN_IF_TNKfrm_01", "SIGN_IF_PETfrm_01",
                                          "SIGN_IF_WENfrm_01", "SIGN_IF_TGLfrm_01", "SIGN_IF_CUBfrm_01" };
    character = n;
    drop_pic(character_pic);
    character_pic = frame_pic(n, names);
    if (!character_pic) return;                      // port: as above
    character_pic->load();
    character_pic->x = tar.x;
    character_pic->y = tar.y;
}

static std::string save_name(int n) {
    char buf[16];
    snprintf(buf, sizeof buf, "save%c", (char)('0' + n));
    return buf;
}

// 0x4b0a50: a frame counts as saved when its file has a character, a location and a skill level.
void SignInFrame::load(int n) {
    Saves &sv = Game::state().saves;
    saved = true;
    sv.open(save_name(n));
    int v = 0;
    if (!sv.getInt("Character", &v)) saved = false;
    else setCharacter(v);
    if (!sv.getInt("Location", &v)) saved = false;
    else setLocation(v);
    if (!sv.getSkill(&v)) saved = false;
    else skill = v;
    bool b = false;
    if (sv.hasTreasure(&b)) bell.visible = b;
    cap.visible = sv.hasCap();
}

// 0x4b0c40: writes the frame's choices, makes it the current player, and reads that player's progress.
void SignInFrame::save(int n) {
    Saves &sv = Game::state().saves;
    sv.open(save_name(n));
    sv.setInt("Character", character);
    sv.setInt("Location", location);
    sv.setSkill(skill);
    Game::state().skill = skill;                           // +0x68
    sv.setInt("Frame Color", n);
    Game::state().frame_color = n;                         // 0x4ef7b0(n); +0x788 = the colour record (port: TODO)
    // port: TODO 0x4e6390, 0x4ed270, 0x4ed9c0, 0x4ee110, 0x4ee860, 0x4eefc0 load each area's progress from the
    // save; they come with the areas' scenes.
    sv.flush();                                      // 0x4e9840
}

// 0x4b0e80: deletes the frame's save and empties it.
void SignInFrame::erase(int n) {
    // port: TODO 0x4c6450 sets a block of the state's flags to 1 first (the state object isn't ported that far).
    Saves &sv = Game::state().saves;
    sv.open(save_name(n));                           // the exe names it "Save\save<n>"; Saves keeps them in Save\.
    sv.deleteCurrent();                              // 0x4e9860
    bell.visible = false;
    cap.visible = false;
    drop_pic(location_pic);
    drop_pic(character_pic);
    location = -1;
    character = -1;
    skill = 0;
    saved = false;
}

// ---------------------------------------------------------------- SignInCursor
SignInCursor::SignInCursor(const char *a, const char *n)   // 0x4afc70
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), anim(a),
      normal(n, 0, 0, 0, 0, 0, 0, true) {
    anim.load();
    anim.start_time = frame_time();                  // 0x403be0(0, 1): started now, looping
    anim.mode = 1;
    normal.load();
}
SignInCursor::~SignInCursor() {                      // 0x4afd70
    anim.unload();
    normal.unload();
}
void SignInCursor::draw() {                          // 0x4afe20
    if (active) {
        anim.x = x; anim.y = y;
        anim.draw();
        return;
    }
    normal.x = x; normal.y = y;
    normal.draw();
}

// ---------------------------------------------------------------- SignInWendy
SignInWendy::SignInWendy() : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {   // 0x4b4310
    ras = ras_open("sign_wen", "", &data_device());  // 0x419b90
    ras->load();                                     // 0x41aa20
    ras_set_current(ras);                            // 0x41a920
    root = new AnimationHelper("SIGN_CA_WENroot_", 2);
    lips = new AnimationHelper("wes_mout", 0);
    root->load();
    root->fps = 12;
    lips->load();
    lips->visible = false;
    AnimationHelper *a = new AnimationHelper("SIGN_CA_WENroottk_", 0);
    a->load();
    a->fps = 12;
    talks.push_back(a);
    for (int i = 1; i < 4; i++) {
        char name[32];
        snprintf(name, sizeof name, "SIGN_CA_WENtalk%d_", i);
        a = new AnimationHelper(name, 0);
        a->load();
        a->fps = 12;
        talks.push_back(a);
    }
    cur = nullptr;
    picked = false;
}

SignInWendy::~SignInWendy() {                        // 0x4b4730
    if (root) { root->unload(); delete root; }
    root = nullptr;
    if (lips) { lips->unload(); delete lips; }
    lips = nullptr;
    cur = nullptr;
    for (size_t i = 0; i < talks.size(); i++) {
        if (talks[i]) { talks[i]->unload(); delete talks[i]; }
    }
    talks.clear();
    if (ras) ras_close(ras);                         // 0x41b050 + 0x419620
    ras = nullptr;
}

// One of the talking animations: half the time a random one, else the first (0x425d40(2), 0x425d40(count)).
void SignInWendy::pickTalk() {
    int r = vision_random(2) == 0 ? vision_random((int)talks.size()) : 0;
    if (r >= (int)talks.size()) return;              // port: the exe throws "invalid vector<T> subscript" (0x4b4be0)
    cur = talks[r];
    cur->forward();                                  // 0x4e31b0
    cur->start();                                    // 0x4e3190
    picked = true;
    state = 2;
}

// 0x4b4990: while the current animation runs it just draws. Idle: the root animation from its first frame again.
// Talking with the state's +0x560 flag set: another talking animation once the last one has ended.
void SignInWendy::draw() {
    bool restart = false;
    if (!(cur && cur->playing())) {
        if (state == 1) restart = true;
        else if (state == 2 && Game::state().vo_suppressed && !picked) pickTalk();
    }
    if (restart) {
        cur = root;
        root->setFrame(0);
        root->start();
        state = 1;
    }
    if (cur) cur->draw();
    if (lips->visible) lips->draw();
}

void SignInWendy::idle() {                           // 0x4b4ac0
    cur = root;
    root->setFrame(0);
    root->start();
    state = 1;
}

bool SignInWendy::ready() {                          // 0x4b4b30
    if (state == 2) return true;
    if (state == 1) {
        if (!picked) pickTalk();
        return true;
    }
    return false;
}

void SignInWendy::mouth(int letter) {                // 0x4b4af0
    lips->visible = true;
    lips->setFrame(letter);
}

void SignInWendy::rest() {                           // 0x4b4b10
    if (cur) { cur->reverse(); cur->start(); }       // 0x4e3220, 0x4e3190
    picked = false;
    lips->visible = false;
}

// ---------------------------------------------------------------- SignIn_Scene
SignIn_Scene::SignIn_Scene()                         // 0x4b1050
    : Scene("signin"), crumple("SIGN_FX_crumple"), screen(0, 0, 0, 0, 0, true, true),
      background("SIGN_IF_background_01", 0, 0, 0, 0, 0, 0, true),
      wall("SIGN_VO_WENwall", 1000), charpt("SIGN_VO_WENcharpt", 3000), levdif("SIGN_VO_WENlevdif", 3000),
      begin("SIGN_VO_WENbegin", 3000), begin2("SIGN_VO_WENbegin2", 3000), music("SIGN_MU_music") {
    frmidl.add("SIGN_VO_WENfrmidl", false);
    frame1.add("SIGN_VO_WENframe1", false);
    bckidl.add("SIGN_VO_WENbckidl", false);
    chridl.add("SIGN_VO_WENchridl", false);
    difidl.add("SIGN_VO_WENdifidl", false);
    clicksg.add("SIGN_VO_WENclicksg", false);
    cliksg2.add("SIGN_VO_WENcliksg2", false);
    Inactivity *all[7] = { &frmidl, &frame1, &bckidl, &chridl, &difidl, &clicksg, &cliksg2 };
    for (int i = 0; i < 7; i++) all[i]->setInterval(15000);   // 0x4c4850(15000)
}

void SignIn_Scene::load2() {                         // 0x4b17c0
    mouse.setClient(this);
    keyboard.setClient(this);
    music.mode = 2;                                  // +0x2a1c: streamed from the file
    music.type = 6;                                  // +0x2a24: loops
}

void SignIn_Scene::enter() {                         // 0x4b1800
    for (int i = 0; i < 8; i++) flags[i] = false;
    ras = ras_open(name(), "", &data_device());      // 0x4243f0 + 0x419b90
    ras->load();                                     // 0x41aa20
    ras_set_current(ras);                            // 0x41a920

    frames[5] = new SignInFrame("SIGN_BU_GREENtar_01", "SIGN_BU_GREENidl_01", "SIGN_BU_GREENrov_01",
                                "SIGN_BU_GREENsel_01", "SIGN_IF_bell_01", "SIGN_IF_cap_01");
    frames[4] = new SignInFrame("SIGN_BU_YELLOWtar_01", "SIGN_BU_YELLOWidl_01", "SIGN_BU_YELLOWrov_01",
                                "SIGN_BU_YELLOWsel_01", "SIGN_IF_bell_02", "SIGN_IF_cap_02");
    frames[2] = new SignInFrame("SIGN_BU_PURPLEtar_01", "SIGN_BU_PURPLEidl_01", "SIGN_BU_PURPLErov_01",
                                "SIGN_BU_PURPLEsel_01", "SIGN_IF_bell_03", "SIGN_IF_cap_03");
    frames[3] = new SignInFrame("SIGN_BU_ORANGEtar_01", "SIGN_BU_ORANGEidl_01", "SIGN_BU_ORANGErov_01",
                                "SIGN_BU_ORANGEsel_01", "SIGN_IF_bell_04", "SIGN_IF_cap_04");
    frames[1] = new SignInFrame("SIGN_BU_BLUEtar_01", "SIGN_BU_BLUEidl_01", "SIGN_BU_BLUErov_01",
                                "SIGN_BU_BLUEsel_01", "SIGN_IF_bell_05", "SIGN_IF_cap_05");
    frames[0] = new SignInFrame("SIGN_BU_REDtar_01", "SIGN_BU_REDidl_01", "SIGN_BU_REDrov_01",
                                "SIGN_BU_REDsel_01", "SIGN_IF_bell_06", "SIGN_IF_cap_06");
    static const char *const L[6] = { "LOND", "SKL", "INDV", "MCOV", "STAR", "LOST" };
    static const char *const Lvo[6] = { "lond", "sklr", "indv", "mcov", "star", "lost" };
    static const char *const C[6] = { "HOK", "TNK", "PET", "WEN", "TGL", "CUB" };
    static const char *const Cvo[6] = { "hok", "tnk", "pet", "wen", "tgl", "cub" };
    char a[40], b[40], c[40], v[40];
    for (int i = 0; i < 6; i++) {
        snprintf(a, sizeof a, "SIGN_BU_%sidl_01", L[i]); snprintf(b, sizeof b, "SIGN_BU_%srov_01", L[i]);
        snprintf(c, sizeof c, "SIGN_BU_%ssel_01", L[i]); snprintf(v, sizeof v, "SIGN_VO_WEN%s", Lvo[i]);
        locs[i] = new SignInButton(a, b, c, v, v);
    }
    for (int i = 0; i < 6; i++) {
        snprintf(a, sizeof a, "SIGN_BU_%sidl_01", C[i]); snprintf(b, sizeof b, "SIGN_BU_%srov_01", C[i]);
        snprintf(c, sizeof c, "SIGN_BU_%ssel_01", C[i]); snprintf(v, sizeof v, "SIGN_VO_WEN%s", Cvo[i]);
        chars[i] = new SignInButton(a, b, c, v, v);
    }
    bigben = new SignInButton("SIGN_BU_BIGBENidl_01", "SIGN_BU_BIGBENrov_01", "SIGN_BU_BIGBENsel_01",
                              "SIGN_VO_WENbigben", "GLOB_FX_click");
    map = new SignInButton("SIGN_BU_MAPidl_01", "SIGN_BU_MAProv_01", "SIGN_BU_MAPsel_01", "SIGN_VO_WENnvmap",
                           "GLOB_FX_click");
    skills[0] = new SignInButton("SIGN_BU_BEGidl_01", "SIGN_BU_BEGrov_01", "SIGN_BU_BEGsel_01",
                                 "SIGN_VO_WENrovbeg", "SIGN_VO_WENrovbeg");
    skills[1] = new SignInButton("SIGN_BU_INTidl_01", "SIGN_BU_INTrov_01", "SIGN_BU_INTsel_01",
                                 "SIGN_VO_WENrovint", "SIGN_VO_WENrovint");
    skills[2] = new SignInButton("SIGN_BU_ADVidl_01", "SIGN_BU_ADVrov_01", "SIGN_BU_ADVsel_01",
                                 "SIGN_VO_WENrovadv", "SIGN_VO_WENrovadv");
    trash = new SignInButton("SIGN_BU_TRAidl_01", "SIGN_BU_TRArov_01", "SIGN_BU_TRAsel_01", nullptr, nullptr);
    sure = false;

    crumple.mode = 1;                                // +0x468
    crumple.type = 7;                                // +0x470
    crumple.open();                                  // 0x41f9d0(0)
    mouse.activate();
    keyboard.activate();
    screen.loadAll();                                // 0x41ec40
    background.load();
    Layer *l0 = screen.layer(0);
    l0->add(&background);
    for (int i = 0; i < 6; i++) l0->add(frames[i]);
    for (int i = 0; i < 6; i++) l0->add(locs[i]);
    for (int i = 0; i < 6; i++) l0->add(chars[i]);
    l0->add(map);
    l0->add(bigben);
    for (int i = 0; i < 3; i++) l0->add(skills[i]);
    trash->enabled = true;
    l0->add(trash);
    reset();                                         // 0x421110
    activate();
    fade.load();
    set_depth(&fade, -10);
    l0->add(&fade);

    cursor = new SignInCursor("GLOB_CU_active_", "GLOB_CU_normal_01");
    cursor->active = false;
    set_cursor(cursor, g_cursor_graphic ? g_cursor_graphic->visible : false);
    show_cursor();
    for (int i = 0; i < 6; i++) frames[i]->load(i);
    Saves &sv = Game::state().saves;
    if (!sv.tutorialsPassed(&tutorials)) {           // 0x4ea4f0
        tutorials = false;
        sv.setTutorialsPassed(false);                // 0x4ea570
    }
    map->visible = tutorials;
    next = "";
    bigben->enabled = false;
    map->enabled = false;
    setEnabled(false, false, false, false);
    screen.activate();
    hide_cursor();
    fade.start();                                    // 0x4e61f0

    wendy = new SignInWendy();
    l0->add(wendy);
    VoiceOver &vo = VoiceOver::get();
    static const char *const lines[] = {
        "SIGN_VO_WENgreet1", "SIGN_VO_WENgreet2", "SIGN_VO_WENfull", "SIGN_VO_WENlond", "SIGN_VO_WENsklr",
        "SIGN_VO_WENmcov", "SIGN_VO_WENindv", "SIGN_VO_WENstar", "SIGN_VO_WENlost", "SIGN_VO_WENhok",
        "SIGN_VO_WENtnk", "SIGN_VO_WENpet", "SIGN_VO_WENtgl", "SIGN_VO_WENcub", "SIGN_VO_WENwen",
        "SIGN_VO_WENrovbeg", "SIGN_VO_WENrovint", "SIGN_VO_WENrovadv", "SIGN_VO_WENsaved", "SIGN_VO_WENusure",
        "SIGN_VO_WENwall", "SIGN_VO_WENcharpt", "SIGN_VO_WENlevdif", "SIGN_VO_WENbegin", "SIGN_VO_WENbegin2",
        "SIGN_VO_WENfrmidl", "SIGN_VO_WENbckidl", "SIGN_VO_WENchridl", "SIGN_VO_WENdifidl", "SIGN_VO_WENcliksg2",
        "SIGN_VO_WENclicksg", "SIGN_VO_WENbigben", "SIGN_VO_WENnvmap", "SIGN_VO_WENnosave", "SIGN_VO_WENframe1",
        "SIGN_VO_WENgreet3" };
    for (size_t i = 0; i < sizeof lines / sizeof lines[0]; i++) vo.registerLine(lines[i], wendy);   // 0x4c91b0
    wendy->idle();
    music.open();                                    // 0x41f9d0(0)
    music.play();                                    // 0x41fd50
}

void SignIn_Scene::leave() {                         // 0x4b2920
    VoiceOver::get().stop();
    wall.stop(); charpt.stop(); levdif.stop(); begin.stop(); begin2.stop();
    frmidl.stop(); frame1.stop(); bckidl.stop(); chridl.stop(); difidl.stop(); clicksg.stop(); cliksg2.stop();
    fade.unload();
    delete cursor;
    if (g_cursor_graphic == cursor) g_cursor_graphic = nullptr;   // port: the exe re-points it below
    cursor = nullptr;
    mouse.deactivate();
    keyboard.deactivate();
    if (wendy) { wendy->unload(); delete wendy; }
    wendy = nullptr;
    for (int i = 0; i < 6; i++) {
        delete frames[i]; frames[i] = nullptr;
        delete locs[i]; locs[i] = nullptr;
        delete chars[i]; chars[i] = nullptr;
    }
    delete bigben; bigben = nullptr;
    delete map; map = nullptr;
    for (int i = 0; i < 3; i++) { delete skills[i]; skills[i] = nullptr; }
    delete trash; trash = nullptr;
    music.close();                                   // 0x41fc50
    screen.unloadAll();                              // 0x41ec00
    screen.clear();                                  // 0x41ebc0
    screen.deactivate();
    if (ras) ras_close(ras);                         // 0x41b050 + 0x419620
    ras = nullptr;
    deactivate();
    crumple.close();
    set_cursor(Game::state().cursor, true);                // the game's cursor (state +0x5c)
}

void SignIn_Scene::stopPrompts() {
    frmidl.stop(); frame1.stop(); bckidl.stop(); chridl.stop(); difidl.stop(); clicksg.stop(); cliksg2.stop();
    wall.stop(); charpt.stop(); levdif.stop(); begin.stop(); begin2.stop();
}

void SignIn_Scene::setEnabled(bool fr, bool lo, bool ch, bool sk) {
    for (int i = 0; i < 6; i++) frames[i]->enabled = fr;
    for (int i = 0; i < 6; i++) locs[i]->enabled = lo;
    for (int i = 0; i < 6; i++) chars[i]->enabled = ch;
    for (int i = 0; i < 3; i++) skills[i]->enabled = sk;
}

// 0x4b2b70: after the fade-in Wendy greets (a new player, a returning one, or a full wall), then the scene polls its
// hints and goes to the chosen scene.
void SignIn_Scene::run() {
    wait(1);
    cursor->active = false;
    while (fade.mode != 2) wait(30);
    bool any = false;
    for (int i = 0; i < 6; i++) if (frames[i]->complete()) { any = true; break; }
    bool greeted3 = false;
    if (any) {
        if (Game::state().returning) {                     // +0x1819
            Game::state().returning = false;
            vo_play("SIGN_VO_WENgreet3");
            greeted3 = true;
        } else {
            vo_play("SIGN_VO_WENgreet2");
            bool all = true;
            for (int i = 0; i < 6; i++) if (!frames[i]->complete()) { all = false; break; }
            if (all) vo_play("SIGN_VO_WENfull");
            frmidl.start();
        }
    } else {
        vo_play("SIGN_VO_WENgreet1");
        frame1.start();
    }
    (void)greeted3;
    for (int i = 0; i < 6; i++) frames[i]->enabled = true;
    show_cursor();
    for (;;) {
        update();
        if (!next.empty()) {
            hide_cursor();
            VoiceOver::get().stop();
            change_scene(next.c_str());              // 0x41ddf0
        }
        wait(30);
    }
}

void SignIn_Scene::notify2(Service *s) {             // 0x4b3e10
    if (s == &mouse) onMouse();
    if (s == &keyboard && g_key.down) {              // 0x5a6dd8
        stopBlinking();
        if (g_key.down) Game::state().key(g_key.vk);       // 0x4baef0: a no-op
    }
}

void SignIn_Scene::track(SignInButton *b, int group, int i) {
    if (!b->track()) return;
    cursor->active = true;
    switch (group) {
    case 0: onFrame(i); break;
    case 1: onLocation(i); break;
    case 2: onCharacter(i); break;
    case 3:                                          // Big Ben: save, then London
        if (b->clicked) {
            frames[slot]->save(slot);
            Game::state().start_in_london = true;          // +0x1818
            next = "london";                         // 0x411840
        }
        break;
    case 4: onMap(); break;
    case 5: onSkill(i); break;
    case 6: onTrash(); break;
    }
}

// 0x4b3970: every mouse event runs over the buttons in this order; the cursor animates over any of them.
void SignIn_Scene::onMouse() {
    if (!g_cursor_graphic || !g_cursor_graphic->visible) return;
    if (mouse_down()) stopBlinking();
    cursor->active = false;
    for (int i = 0; i < 6; i++) track(frames[i], 0, i);
    for (int i = 0; i < 6; i++) track(locs[i], 1, i);
    for (int i = 0; i < 6; i++) track(chars[i], 2, i);
    track(bigben, 3, 0);
    track(map, 4, 0);
    for (int i = 0; i < 3; i++) track(skills[i], 5, i);
    track(trash, 6, 0);
}

// 0x4b2dd0: a frame chosen. Leaving an unsaved frame empties it. A saved frame offers its skill and the way in; an
// empty one asks for a place.
void SignIn_Scene::onFrame(int i) {
    if (!frames[i]->clicked) return;
    sure = false;
    if (0 <= slot && slot < 6) {
        frames[slot]->sel.visible = false;
        SignInFrame *f = frames[slot];
        if (!f->saved) {
            drop_pic(f->location_pic);
            drop_pic(f->character_pic);
            f->location = -1;
            f->character = -1;
            f->saved = false;
        }
    }
    slot = i;
    frames[i]->sel.visible = true;
    if (frames[slot]->saved) {
        bigben->enabled = true;
        map->enabled = true;
        setEnabled(true, false, false, true);
        for (int k = 0; k < 3; k++) skills[k]->sel.visible = false;
        skill_sel = frames[slot]->skill - 1;
        skills[skill_sel]->sel.visible = true;
        vo_play("SIGN_VO_WENsaved");
        return;
    }
    bigben->enabled = false;
    map->enabled = false;
    setEnabled(true, true, false, false);
    stopPrompts();
    wall.start();
    bckidl.start();
}

void SignIn_Scene::onLocation(int i) {               // 0x4b30c0
    if (!locs[i]->clicked) return;
    if (0 <= loc && loc < 6) locs[loc]->sel.visible = false;
    loc = i;
    locs[i]->sel.visible = true;
    frames[slot]->setLocation(i);
    stopPrompts();
    charpt.start();
    chridl.start();
    static const char *const vo[6] = { "SIGN_VO_WENlond", "SIGN_VO_WENsklr", "SIGN_VO_WENindv",
                                       "SIGN_VO_WENmcov", "SIGN_VO_WENstar", "SIGN_VO_WENlost" };
    vo_play(vo[i]);
    bigben->enabled = false;
    map->enabled = false;
    setEnabled(false, true, true, false);
}

void SignIn_Scene::onCharacter(int i) {              // 0x4b3300
    if (!chars[i]->clicked) return;
    if (0 <= chr && chr < 6) chars[chr]->sel.visible = false;
    chr = i;
    chars[i]->sel.visible = true;
    stopPrompts();
    levdif.start();
    difidl.start();
    static const char *const vo[6] = { "SIGN_VO_WENhok", "SIGN_VO_WENtnk", "SIGN_VO_WENpet",
                                       "SIGN_VO_WENwen", "SIGN_VO_WENtgl", "SIGN_VO_WENcub" };
    vo_play(vo[i]);
    frames[slot]->setCharacter(i);
    bigben->enabled = false;
    map->enabled = false;
    setEnabled(false, false, true, true);
}

// 0x4b3540: the skill level; then the way in (Big Ben, and the map once the tutorials are passed).
void SignIn_Scene::onSkill(int i) {
    if (!skills[i]->clicked) return;
    if (0 <= skill_sel && skill_sel < 3) skills[skill_sel]->sel.visible = false;
    skill_sel = i;
    skills[i]->sel.visible = true;
    for (int k = 0; k < 6; k++) chars[k]->enabled = false;
    frmidl.stop(); frame1.stop(); bckidl.stop(); chridl.stop(); difidl.stop(); clicksg.stop(); cliksg2.stop();
    wall.stop(); charpt.stop(); levdif.stop(); begin.stop(); begin2.stop();
    if (!tutorials) {
        begin.start();
        clicksg.start();
    } else {
        begin2.start();
        cliksg2.start();
    }
    if (i == 0) vo_play("SIGN_VO_WENrovbeg");
    else if (i == 1) vo_play("SIGN_VO_WENrovint");
    else if (i == 2) vo_play("SIGN_VO_WENrovadv");
    frames[slot]->skill = i + 1;
    for (int k = 0; k < 6; k++) if (frames[k]->saved) frames[k]->enabled = true;
    bigben->enabled = true;
    if (tutorials) map->enabled = true;
}

void SignIn_Scene::onMap() {                         // 0x4b3780
    if (!map->clicked) return;
    frames[slot]->save(slot);
    next = "map";
}

// 0x4b37d0: with no frame chosen Wendy says there's nothing to delete; the first press asks, the second deletes.
void SignIn_Scene::onTrash() {
    if (!trash->clicked) return;
    if (slot == -1) { vo_play("SIGN_VO_WENnosave"); return; }
    if (!sure) {
        vo_play("SIGN_VO_WENusure");
        sure = true;
        return;
    }
    crumple.play();
    VoiceOver::get().stop();
    sure = false;
    frames[slot]->erase(slot);
    bigben->enabled = false;
    map->enabled = false;
    setEnabled(true, false, false, false);
    slot = -1;
    for (int k = 0; k < 6; k++) frames[k]->sel.visible = false;
    for (int k = 0; k < 6; k++) locs[k]->sel.visible = false;
    for (int k = 0; k < 6; k++) chars[k]->sel.visible = false;
    for (int k = 0; k < 3; k++) skills[k]->sel.visible = false;
}

// 0x4b3e70: when a prompt starts or ends, the buttons it talks about blink.
void SignIn_Scene::update() {
    uint32_t now;
    if (vo_isPlaying("SIGN_VO_WENgreet1") && !flags[0]) {      // the greeting: the frames, 7.5 s in
        for (int i = 0; i < 6; i++) frames[i]->blink = frame_time() + 7500;
        flags[0] = true;
    }
    if (!vo_isPlaying("SIGN_VO_WENgreet1") && flags[0]) flags[0] = false;

    if (vo_isPlaying(wall.line.c_str())) flags[1] = true;      // 0x4c17d0: "pick a place": the places
    if (!vo_isPlaying(wall.line.c_str()) && flags[1]) {
        for (int i = 0; i < 6; i++) locs[i]->blink = frame_time();
        flags[1] = false;
    }
    if (vo_isPlaying(charpt.line.c_str())) flags[2] = true;    // the characters
    if (!vo_isPlaying(charpt.line.c_str()) && flags[2]) {
        for (int i = 0; i < 6; i++) chars[i]->blink = frame_time();
        flags[2] = false;
    }
    if (vo_isPlaying(levdif.line.c_str())) flags[3] = true;    // the skill levels
    if (!vo_isPlaying(levdif.line.c_str()) && flags[3]) {
        for (int i = 0; i < 3; i++) skills[i]->blink = frame_time();
        flags[3] = false;
    }
    if (vo_isPlaying(begin.line.c_str()) && !flags[4]) {       // Big Ben, 2.5 s in
        bigben->blink = frame_time() + 2500;
        flags[4] = true;
    }
    if (!vo_isPlaying(begin.line.c_str()) && flags[4]) flags[4] = false;
    if (vo_isPlaying(begin2.line.c_str()) && !flags[5]) {      // Big Ben 3 s in, then the map
        bigben->blink = frame_time() + 3000;
        flags[5] = true;
    }
    if (!vo_isPlaying(begin2.line.c_str()) && flags[5]) {
        map->blink = frame_time();
        flags[5] = false;
    }
    if (clicksg.isPlaying()) flags[6] = true;                  // 0x4c4aa0
    if (!clicksg.isPlaying() && flags[6]) {
        now = frame_time();
        bigben->blink = now + 1500;
        flags[6] = false;
    }
    if (cliksg2.isPlaying() && !flags[7]) {
        bigben->blink = frame_time() + 1500;
        flags[7] = true;
    }
    if (!cliksg2.isPlaying() && flags[7]) {
        map->blink = frame_time() + 1500;
        flags[7] = false;
    }
}

void SignIn_Scene::stopBlinking() {                  // 0x4b4160
    for (int i = 0; i < 6; i++) frames[i]->blink = 0;
    for (int i = 0; i < 6; i++) locs[i]->blink = 0;
    for (int i = 0; i < 6; i++) chars[i]->blink = 0;
    for (int i = 0; i < 3; i++) skills[i]->blink = 0;
    map->blink = 0;
    bigben->blink = 0;
    trash->blink = 0;
    for (int i = 0; i < 8; i++) flags[i] = false;
}

}  // namespace Game
