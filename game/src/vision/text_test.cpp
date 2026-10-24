// Lane C's desktop test scene for the text classes: `TB_TEST_TEXT=1 tinkerbell texttest`. Not part of the game.
#include "text.h"
#include <stdlib.h>

namespace Vision {
namespace {
struct TextTestScene : Scene {
    Screen screen;
    Rectangle bg, solid, frame_in, frame_out, blend;
    Text left, centre, right, wrapped, boxed;
    TextField field;
    TextTestScene()
        : Scene("texttest"), screen(0, 0, 0, 0, 0, true, true), bg(0, 0, 0, 640, 480, true, 0xff804020u),
          solid(-200, -150, 0, 120, 60, true, 0xff00c0ffu), frame_in(-60, -150, 0, 120, 60, true, 0xffffffffu),
          frame_out(80, -150, 0, 120, 60, true, 0xff40ff40u), blend(-40, -130, 0, 200, 30, true, 0x80ffffffu),
          left(-200, -60, 0, 200, 24, true, 0xffffffffu), centre(0, -30, 0, 200, 24, true, 0xff00ffffu),
          right(200, -60, 0, 200, 24, true, 0xffffff00u), wrapped(-150, 40, 0, 160, 80, true, 0xffffffffu),
          boxed(120, 40, 0, 160, 40, true, 0xffffffffu) {}
    void enter() override {
        frame_in.f78 = 3; frame_out.f78 = -3;
        left.halign = -1; left.setText("Left aligned text");
        centre.setText("Centred: Tinker Bell");
        right.halign = 1; right.setText("Right 12345");
        wrapped.wrap = true; wrapped.valign = -1; wrapped.halign = -1;
        wrapped.setText("A longer sentence that wraps inside its box.");
        boxed.bk = 0xff602020u; boxed.setText("Boxed");
        field.x = 0; field.y = 150; field.w = 300; field.h = 30; field.color = 0xffffffffu;
        field.impl->centered = false;
        screen.loadAll();
        for (Graphic *g : {(Graphic *)&bg, (Graphic *)&solid, (Graphic *)&frame_in, (Graphic *)&frame_out, (Graphic *)&blend,
                           (Graphic *)&left, (Graphic *)&centre, (Graphic *)&right, (Graphic *)&wrapped, (Graphic *)&boxed,
                           (Graphic *)&field}) {
            g->load();
            screen.layer(0)->add(g);
        }
        field.setText("Wendy");
        field.impl->kbd.activate();
        field.impl->focus = true;
        field.impl->caret->visible = true;
        for (const char *p = "ab"; *p; p++) key_event((unsigned char)*p, false, true, true, false);   // typed through the keyboard service
        key_event(0x25, false, true, false, false);                                                // left arrow
        screen.activate();
        activate();
    }
    void leave() override { screen.deactivate(); deactivate(); }
};
struct Init { Init() { if (getenv("TB_TEST_TEXT")) new TextTestScene(); } } g_init;
}  // namespace
}  // namespace Vision
