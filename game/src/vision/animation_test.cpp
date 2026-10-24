// Lane A's desktop test for Animation: `TB_TEST_ANIM=<wad>:<prefix>[:mode] tinkerbell animtest`. Not part of the game.
// Plays the animation (loop by default) centred on a grey background; the headless run's TB_SHOTS saves frames.
#include "animation.h"
#include "text.h"
#include <stdlib.h>
#include <string.h>

namespace Vision {
namespace {
struct AnimTestScene : Scene {
    Screen screen;
    Rectangle bg;
    std::string wad, pre;
    int mode;
    Animation *a = nullptr;
    AnimTestScene(const char *spec) : Scene("animtest"), screen(0, 0, 0, 0, 0, true, true), bg(0, 0, 0, 640, 480, true, 0xff808080u), mode(1) {
        std::string s = spec;
        size_t c = s.find(':');
        wad = s.substr(0, c);
        if (c != std::string::npos) pre = s.substr(c + 1);
        c = pre.find(':');
        if (c != std::string::npos) { mode = atoi(pre.c_str() + c + 1); pre = pre.substr(0, c); }
        size_t d = wad.rfind(".wad");
        if (d != std::string::npos) wad = wad.substr(0, d);
    }
    void enter() override {
        Ras *r = ras_open(wad.c_str(), "", &data_device());
        ras_set_current(r);
        screen.loadAll();
        bg.load();
        bg.z = -30000;                       // behind the frames
        if (!getenv("TB_ANIM_NOBG")) screen.layer(0)->add(&bg);
        a = new AnimationHelper(pre.c_str(), mode);
        vision_log("ANIMTEST %s:%s frames=%d box=%d,%d %dx%d fps=%d", wad.c_str(), pre.c_str(), a->count(), a->x, a->y, a->w, a->h, a->fps);
        { Image *f = a->frame(0); vision_log("ANIMTEST f0 x=%d y=%d z=%d vis=%d pm=%p bpp=%d kind=%d anim.z=%d vis=%d", f->x, f->y, f->z, (int)f->visible, (void*)f->pixelmap, f->pixelmap?f->pixelmap->bpp:-1, f->pixelmap?f->pixelmap->kind:-1, a->z, (int)a->visible); }
        for (Image *f : a->frames) vision_log("ANIMTEST   %s x=%d y=%d w=%d h=%d pmx=%d pmy=%d", f->name(), f->x, f->y, f->w, f->h, f->pixelmap->x, f->pixelmap->y);
        if (getenv("TB_ANIM_IMG")) { Image *im = new Image(getenv("TB_ANIM_IMG"), 0, 0, 0, 0, 0, 0, true); im->load(); screen.layer(0)->add(im); }
        else screen.layer(0)->add(a);
        a->start();
        screen.activate();
        activate();
    }
    void run() override {
        for (int i = 0; i < 400; i++) {
            wait(100);
            if (i % 5 == 0) vision_log("ANIMTEST t=%d frame=%d playing=%d", i * 100, a->current(), (int)a->playing());
        }
        for (;;) wait(1000);
    }
    void leave() override { screen.deactivate(); deactivate(); }
};
struct Init { Init() { if (const char *s = getenv("TB_TEST_ANIM")) new AnimTestScene(s); } } g_init;
}  // namespace
}  // namespace Vision
