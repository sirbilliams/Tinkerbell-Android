// Text on screen (lane C): the Font object, Text (a Graphic drawn through GDI), Rectangle, TextField (an editable
// text box, with Impl@TextField as its Task) and Joystick, reconstructed from "Tinker Bell.exe".
// The text is drawn on a DC of the back buffer (canvas mode 2, 0x40e390): platform/gdi.h.
#pragma once
#include "engine.h"
#include "../../../platform/gdi.h"
#include <string>

namespace Vision {

// ---------------------------------------------------------------- the DC of the canvas (0x5a688c)
TbHDC canvas_dc();                                   // 0x40ea50
void canvas_dc_begin(Canvas *c);                     // 0x40e390, mode 2: GetDC + SetViewportOrgEx(w/2 - ox, h/2 - oy)
void canvas_dc_end();                                // 0x40e620, mode 2: ReleaseDC
void canvas_set_mode(int mode);                      // graphics.cpp: 1 = pixels locked, 2 = a DC

// ---------------------------------------------------------------- Font (vtable 0x51354c, 0x3c bytes)
// The exe has one of these, the default font at 0x5a6938 (no face, size 0 = the stock font 0x10); every Text
// points at it (+0x98).
class Font : public Object {
public:
    // 0x415ca0(face, size, orientation, italic, underline, weight, f38)
    Font(const char *face, int size, int orientation, bool italic, bool underline, int weight, int f38);
    ~Font() override;                                // 0x4125f0
    void create();                                   // 0x4124b0: (re)creates the HFONT
    TbHGDIOBJ hfont = nullptr;                       // +0x24
    std::string face;                                // +0x28 string
    int size;                                        // +0x34 points
    int f38;                                         // +0x38 (what Text +0xa8 starts as: *0x5a6970)
    int orientation;                                 // +0x3c
    int height = 0;                                  // +0x40 -MulDiv(size, logpixels, 72)
    bool italic, underline;                          // +0x44 +0x45
    int weight;                                      // +0x48
};
Font &default_font();                                // 0x5a6938

// ---------------------------------------------------------------- Text (vtable 0x5133bc, 0xc0 bytes)
class Text : public Graphic {
public:
    Text(int x = 0, int y = 0, int z = 0, int w = 0, int h = 0, bool visible = true, uint32_t color = 0xff000000u);
    Text(const Text &o);                             // 0x4028c0 (Object::clone)
    ~Text() override;                                // 0x4029f0
    int canvasMode() override { return 2; }          // +0x10 0x4029e0
    void draw() override;                            // +0x38 0x413bc0
    Text *clone() const { return new Text(*this); }

    void setText(const char *s) { str = s ? s : ""; dirty(); }
    // The measuring / editing helpers the editable TextField uses (all through a screen DC with the font selected):
    void measure();                                  // 0x413110: w/h from the text, keeps the aligned edge fixed
    int maxLineWidth();                              // 0x413320
    int totalHeight();                               // 0x413440: line height x number of lines
    int lineCenterY(int line);                       // 0x4134b0
    int hitLine(int y);                              // 0x4135d0: -1 when not on a line
    int hitCol(int line, int x);                     // 0x4136d0: -1 when left of the line
    bool hitTest(int x, int y, int *line, int *col); // 0x413a80
    bool charPos(int line, int col, int *x, int *y); // 0x4138a0
    bool lineInfo(int line, int *start, int *len);   // 0x413ac0
    int index(int line, int col);                    // 0x413b50: index in str, -1 if none
    int lineCount() const;                           // 0x413bb0: '\n' count + 1

    Font *font;                                      // +0x98
    std::string str;                                 // +0x9c (length, capacity, data)
    int f_a8;                                        // +0xa8 (starts as the font's +0x38)
    int valign = 0;                                  // +0xac: -1 top, 0 middle, 1 bottom (DT_ flags 0 / 4 / 8)
    int halign = 0;                                  // +0xb0: -1 left, 0 centre, 1 right
    uint32_t bk = 0;                                 // +0xb4: B, G, R, and the byte at +0xb7 = draw the background
    bool wrap = false;                               // +0xb8: word break (DT_WORDBREAK)
};

// ---------------------------------------------------------------- Rectangle (vtable 0x5132f4)
class Rectangle : public Graphic {
public:
    Rectangle(int x = 0, int y = 0, int z = 0, int w = 0, int h = 0, bool visible = true, uint32_t color = 0xff000000u);
    void draw() override;                            // +0x38 0x412870: f78 = 0 filled, > 0 inner frame, < 0 outer frame
};
// 0x412db0: fills (or alpha-blends, a < 255) a rectangle of RGB565 pixels; x, y from the canvas's top-left.
void fill_rect565(int x, int y, int w, int h, int c0, int c1, int c2, int a, int cw, int ch, uint16_t *pixels, int pitch);

// ---------------------------------------------------------------- TextField (vtable 0x5132a8) + Impl (0x51338c)
class TextField;
// Impl@TextField (vtable 0x51338c, 0x230 bytes): the Task that blinks the caret, plus the Mouse and Keyboard
// services (its own clients) that edit the text. Offsets are the exe's.
class TextFieldImpl : public Task {
public:
    TextFieldImpl(TextField *owner, Graphic *caret);     // 0x401740
    ~TextFieldImpl() override;                           // 0x401940
    void run() override;                                 // +0x28 0x402670: blinks the caret
    void notify(Service *s) override;                    // +0x20 0x402720
    void click();                                        // 0x401a30
    void key();                                          // 0x401c10
    bool moveRight();                                    // 0x402050
    bool moveLeft();                                     // 0x401ff0
    void clampCol();                                     // 0x401fb0
    bool atEnd();                                        // 0x401f40
    void reflow(bool typed);                             // 0x402160
    bool accepts(int c);                                 // 0x4024a0
    void insert(int c);                                  // 0x402560
    void eraseAt(int idx);                               // 0x424250
    TextField *owner;                                    // +0xc0
    Mouse mouse;                                         // +0xc4
    Keyboard kbd;                                        // +0xf8
    Text text;                                           // +0x130
    int prevLen = 0;                                     // +0x1f0
    bool active_ = false;                                // +0x1f4
    int blink = 500;                                     // +0x1f8 ms (port: the value was not recoverable from the decompile)
    bool ownsCaret = true;                               // +0x1fc
    Graphic *caret;                                      // +0x200
    bool notifyClient = false;                           // +0x21c
    int line = 0, col = 0, anchor = 0;                   // +0x20c +0x210 +0x214
    bool multiline = false;                              // +0x218
    bool focus = true;                                   // +0x219
    bool centered = false;                               // +0x220: the text is centred in the box
    int filter = 0;                                      // +0x224: 0 any, 1 digits, 2 alphanumeric, 3 alphanumeric and '*'
    int maxLen = -1;                                     // +0x228
};

class TextField : public Graphic {
public:
    TextField();                                     // 0x4011b0
    ~TextField() override;                           // 0x401320
    void draw() override;                            // +0x38 0x4015b0
    void load() override;                            // +0x3c 0x4014b0
    void unload() override;                          // +0x40 0x401540
    int canvasMode() override { return 1; }          // +0x10 0x4aada0
    void setText(const char *s);                     // 0x4013e0: clears, then types each character
    const char *getText();                           // 0x401490
    bool focused();                                  // 0x401470: the keyboard service is active
    void hide();                                     // 0x4013a0
    bool enter_ends = false;                         // +0x98: Enter does not type a newline
    TextFieldImpl *impl;                             // +0x9c
};

// ---------------------------------------------------------------- Joystick (vtable 0x515644)
// An empty Service shell like Keyboard / Mouse (never fed: the game reads no joystick, DirectInput makes no device).
class Joystick : public Service {
public:
    Joystick() : Service(nullptr) {}
    Node *root() override;                           // 0x48ed60 -> the list at 0x5a6c50
};

}  // namespace Vision
