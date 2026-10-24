// My_Stage, the two movie scenes ("Bumper", "Intro") and the sign-in room (first pass), reconstructed.
#include "game.h"
#include "../vision/engine.h"
#include "../vision/qtmovie.h"
#include "../vision/sound.h"
#include "../../../platform/ddraw.h"
#include "../../../platform/dsound.h"
#include "tb.h"
#include <string.h>

using namespace Vision;

namespace {

// ---------------------------------------------------------------- My_Stage (vtable 0x5179ec)
struct My_Stage : Stage {
    // 0x4e72a0: Stage("Tinker Bell", "Bumper", *0x543310 = "Tinker Bell\setup\game", 640, 480)
    My_Stage() : Stage("Tinker Bell", "Bumper", "Tinker Bell\\setup\\game", 640, 480) {}
    // +0x10 0x4e72d0: creates the game's global state object (0x1840 bytes, 0x4ec6e0) into 0x63171c.
    // port: TODO (lane A/scenes): the game state (saves, tokens, the "lost" default) is not ported yet.
    void afterServices() override {}
};

// The movie being shown (QuickTime drew into the window by itself; the port presents its frames).
QTMovie *g_movie = nullptr;

// ---------------------------------------------------------------- MovieScene (vtable 0x5155e4)
struct MovieScene : Scene {
    Keyboard keyboard;                       // +0xcc
    QTMovie movie;                           // +0x100
    std::string next;                        // +0x138
    Image image;                             // +0x148 (unnamed, never drawn)
    Screen screen;                           // +0x1f0
    // 0x48b230(name, movie, next)
    MovieScene(const char *n, const char *mov, const char *nxt)
        : Scene(n), movie(mov, "data"), next(nxt), image("", 0, 0, 0, 0, 0, 0, true),
          screen(0, 0, 0, 0, 0, true, true) {}
    void enter() override {                  // +0x4c 0x48b430
        keyboard.activate();
        keyboard.setClient(this);
        movie.open();
        activate();
    }
    void leave() override {                  // +0x50 0x48b470
        keyboard.deactivate();
        movie.dispose();
        deactivate();
    }
    void notify(Service *s) override {       // +0x24 0x48b4a0: Space skips the movie
        if (s == &keyboard && g_key.vk == 0x20) movie.stop();
    }
    void run() override {                    // +0x28 0x48b4d0
        wait(1);
        g_movie_playing = true;              // 0x5a73bc: the Screen stops drawing
        movie.play();
        g_movie = &movie;
        while (movie.isPlaying()) {
            wait(100);                       // port: the movie service presents at its own pace in between
            if (g_quit) break;
        }
        g_movie = nullptr;
        g_movie_playing = false;
        change_scene(next.c_str());
        finish();
    }
};

// The QTMovie service (vtable 0x513e68): MoviesTask for every movie (0x4355b0). port: also presents the frame.
struct QTMovieKind : ServiceKind {
    QTMovieKind() : ServiceKind("QTMovie", 0) {}
    void frame() override {
        if (!g_movie) return;
        g_movie->task();
        const uint16_t *f = g_movie->frame();
        int w, h;
        g_movie->size(&w, &h);
        if (f && w > 0) tb_present_565(f, w, h, w * 2);
        else {                               // no FFmpeg / no frame yet: keep the frame counter going
            static std::vector<uint16_t> black(640 * 480);
            tb_present_565(black.data(), 640, 480, 1280);
        }
    }
};

// The Sound service (vtable 0x513ad4): DirectSoundCreate at init (0x41f8b0), the stream updates each frame
// (0x420600), Release at shutdown (0x4205e0). Lane B's classes do the work.
struct SoundKind : ServiceKind {
    SoundKind() : ServiceKind("Sound", 0) {}
    void init() override { if (!tb_dsound_create()) vision_log("DirectSoundCreate failed"); }
    void frame() override { Sound::tickAll(); }
    void shutdown() override { tb_dsound_destroy(); }
};

// ---------------------------------------------------------------- SignIn_Scene (vtable 0x5168f0), first pass
// 0x4b1050 ctor / 0x4b1800 enter (v19) / 0x4b2920 leave (v20). Only the room itself is drawn so far.
// port: TODO the buttons (0x4b0360 / 0x4afe70), Wendy (0x4b4310), the cursor (0x4afc70), the name entry, the
// voice-overs (0x4c1530 / 0x4c46e0) and the save slots (0x4ea4f0).
struct SignIn_Scene : Scene {
    Sound crumple;                           // +0x130 "SIGN_FX_crumple"
    Screen screen;                           // +0x478
    Mouse mouse;                             // +0x4f4
    Keyboard keyboard;                       // +0x528
    Image background;                        // +0x560
    Ras *ras = nullptr;                      // +0x55c
    SignIn_Scene() : Scene("signin"), crumple("SIGN_FX_crumple"), screen(0, 0, 0, 0, 0, true, true),
                     background("SIGN_IF_background_01", 0, 0, 0, 0, 0, 0, true) {}
    void enter() override {                  // 0x4b1800
        ras = ras_open(name(), "", &data_device());
        ras->load();                         // 0x41aa20
        ras_set_current(ras);                // 0x41a920
        mouse.activate(); mouse.setClient(this);
        keyboard.activate(); keyboard.setClient(this);
        screen.loadAll();
        background.load();                   // +0x3c
        screen.layer(0)->add(&background);
        screen.activate();
        reset();
        activate();
    }
    void leave() override {                  // 0x4b2920
        background.unload();
        mouse.deactivate();
        keyboard.deactivate();
        screen.deactivate();
        deactivate();
    }
};

}  // namespace

namespace Vision { void register_scene_task_kinds(); void register_screen_kind(); }

// The exe's static constructors, in their order (initlist 0x532004): the service kinds (Input, Keyboard,
// Mouse, Ras, Scene, Screen, Sound, Task, QTMovie), the scenes, My_Stage.
void tb_game_register() {
    register_scene_task_kinds();
    register_screen_kind();
    static SoundKind sound;
    static QTMovieKind qt;
    static MovieScene bumper("Bumper", "DIbumper", "Intro");          // 0x48aee0
    static MovieScene intro("Intro", "LOND_MO_intro", "signin");     // 0x48b150
    static SignIn_Scene signin;                                       // 0x4afc40
    static My_Stage st;                                               // 0x4e7280
}
