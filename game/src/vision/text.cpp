// Font, Text and the canvas DC (lane C). See text.h. The GDI calls are the shim's (platform/gdi.h): the original's
// "(*redirected_api)" call sites are named in docs/api-sites.md.
#include "text.h"
#include "tb.h"
#include "../../../platform/ddraw.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>

namespace Vision {

// ---------------------------------------------------------------- the DC of the canvas
static TbHDC g_dc = nullptr;                         // 0x5a688c
static TbSurface *g_dc_surface = nullptr;

TbHDC canvas_dc() { return g_dc; }                   // 0x40ea50

void canvas_dc_begin(Canvas *c) {                    // 0x40e390 (mode 2)
    if (!c || !c->surface || g_dc) return;
    uint16_t *px = nullptr; int pitch = 0;
    c->surface->Lock(&px, &pitch);                   // port: GetDC on the surface = its memory behind a DC
    TbSurfaceBits b = { px, pitch, c->surface->width, c->surface->height, 16, 0xf800, 0x07e0, 0x001f };
    g_dc = tb_gdi_surface_dc(b);
    g_dc_surface = c->surface;
    tb_SetViewportOrgEx(g_dc, c->w / 2 - c->ox, c->h / 2 - c->oy, nullptr);   // the origin is the screen's centre
}
void canvas_dc_end() {                               // 0x40e620 (mode 2): IDirectDrawSurface::ReleaseDC
    if (!g_dc) return;
    tb_gdi_release_surface_dc(g_dc);
    g_dc = nullptr;
    if (g_dc_surface) g_dc_surface->Unlock();
    g_dc_surface = nullptr;
}

// ---------------------------------------------------------------- Font
Font::Font(const char *f, int sz, int orient, bool it, bool ul, int wt, int f38_)    // 0x415ca0
    : face(f ? f : ""), size(sz), f38(f38_), orientation(orient), italic(it), underline(ul), weight(wt) {
    create();
}
Font::~Font() {                                      // 0x4125f0
    if (hfont) {
        if (this != &default_font()) tb_DeleteObject(hfont);   // the stock font is not deleted
        hfont = nullptr;
    }
}
void Font::create() {                                // 0x4124b0
    if (hfont) tb_DeleteObject(hfont);
    if (face.empty() && size == 0) {                 // no face and no size: the stock font
        hfont = tb_GetStockObject(TB_SYSTEM_FIXED_FONT);
        return;
    }
    TbHDC dc = tb_GetDC(nullptr);
    int logpix = tb_GetDeviceCaps(dc, 90);           // LOGPIXELSY
    int h = tb_MulDiv(size, logpix, 72);
    height = -h;
    // 0x5a69c9 is never set, so the original always takes CreateFontA (the other branch enumerates the faces with
    // EnumFontFamiliesExA and creates the matching LOGFONT; the callback is 0x412430).
    hfont = tb_CreateFontA(-h, 0, 0, orientation, weight, italic, underline, 0, 0, 0, 0, 0, 0, face.c_str());
    tb_ReleaseDC(nullptr, dc);
}
Font &default_font() {                               // 0x5a6938, built by the static constructor at 0x4123e0
    static Font f("", 0, 0, false, false, 0, 0);
    return f;
}

// ---------------------------------------------------------------- Text
Text::Text(int x_, int y_, int z_, int w_, int h_, bool vis, uint32_t col)   // e.g. 0x48d470: Graphic(...), font = 0x5a6938
    : Graphic(x_, y_, z_, w_, h_, 0, vis, col, 0, 0, 0, -1, 0xffff00ffu), font(&default_font()), f_a8(default_font().f38) {}

Text::Text(const Text &o)                            // 0x4028c0 (Graphic copy 0x411c50, then the text's own fields)
    : Graphic(o.x, o.y, o.z, o.w, o.h, o.f6c, o.visible, o.color, o.f78, o.f80, o.rotation, o.hitmode, o.f8c),
      font(o.font), str(o.str), f_a8(o.f_a8), valign(o.valign), halign(o.halign), bk(o.bk), wrap(o.wrap) {
    ox = o.ox; oy = o.oy; oz = o.oz; ow = o.ow; oh = o.oh; scale = o.scale; translucency = o.translucency; f90 = o.f90;
}
Text::~Text() {}                                     // 0x4029f0

// COLORREF of a Graphic colour: bytes +0x70, +0x71, +0x72 -> B, G, R (0x413bc0: CONCAT21(CONCAT11(+0x70, +0x71), +0x72))
static uint32_t colorref(uint32_t c) { return ((c & 0xff) << 16) | (c & 0xff00) | ((c >> 16) & 0xff); }

// 0x413bc0 Text::v14
void Text::draw() {
    TbHDC dc = canvas_dc();                          // 0x40ea50
    if (!dc) return;                                 // port: not in a DC canvas mode
    uint32_t tc = colorref(color);
    uint32_t oldColor = tb_SetTextColor(dc, tc);
    int oldMode = tb_SetBkMode(dc, 1);               // TRANSPARENT
    TbHGDIOBJ oldFont = font->hfont ? tb_SelectObject(dc, font->hfont) : nullptr;
    TbRect rc;
    rc.left = x - w / 2; rc.right = x + w / 2;
    rc.bottom = y + h / 2; rc.top = y - h / 2;
    uint32_t flags;
    if (!wrap) flags = (valign == 0 ? 4 : (valign >= 0 ? 8 : 0)) | 0x900;       // DT_VCENTER / DT_BOTTOM / top
    else flags = 0x910;                                                          // DT_WORDBREAK
    flags |= halign == 0 ? 1 : (halign >= 0 ? 2 : 0);                            // DT_CENTER / DT_RIGHT / left
    if ((bk >> 24) != 0) {                           // +0xb7: a background (f78 = 0) or a frame (f78 > 0)
        uint32_t bc = colorref(bk);
        TbHGDIOBJ oldPen, oldBrush, pen, brush;
        if (f78 == 0) {
            TbLogBrush lb = { 0, bc, 0 };            // BS_SOLID
            brush = tb_CreateBrushIndirect(&lb);
            oldBrush = tb_SelectObject(dc, brush);
            pen = tb_CreatePen(6, 0, bc);            // PS_INSIDEFRAME
            oldPen = tb_SelectObject(dc, pen);
        } else {
            pen = tb_CreatePen(6, f78 - 1, bc);
            oldPen = tb_SelectObject(dc, pen);
            brush = tb_GetStockObject(TB_NULL_BRUSH);
            oldBrush = tb_SelectObject(dc, brush);
        }
        int e = f78 > 0 ? f78 - 1 : 0;               // left/top move out by f78 - 1, right/bottom too
        tb_Rectangle(dc, left() - e, top() - e, right() + e, bottom() + e);
        if (oldPen) tb_DeleteObject(tb_SelectObject(dc, oldPen));
        if (oldBrush) tb_DeleteObject(tb_SelectObject(dc, oldBrush));
        (void)pen; (void)brush;
    }
    tb_DrawTextA(dc, str.empty() ? "" : str.c_str(), (int)str.size(), &rc, flags);
    tb_DrawTextA(dc, " ", 1, &rc, flags);            // 0x413e4f: the same call with the one-space string at 0x534f1c
    if (oldFont) tb_SelectObject(dc, oldFont);
    tb_SetBkMode(dc, oldMode);
    tb_SetTextColor(dc, oldColor);
}

// ---- the measuring helpers: a screen DC with the font selected (GetDC(0) / SelectObject / ... / ReleaseDC(0))
namespace {
struct Measure {
    TbHDC dc; TbHGDIOBJ old;
    explicit Measure(Text *t) { dc = tb_GetDC(nullptr); old = tb_SelectObject(dc, t->font->hfont); }
    ~Measure() { tb_SelectObject(dc, old); tb_ReleaseDC(nullptr, dc); }
    TbSize extent(const char *s, int n) { TbSize z = { 0, 0 }; tb_GetTextExtentPoint32A(dc, s, n, &z); return z; }
};
inline int half(int v) { return v / 2; }             // C division, rounding toward zero (cdq / sub / sar)
}

int Text::lineCount() const { return (int)std::count(str.begin(), str.end(), '\n') + 1; }   // 0x413bb0 (0x4241a0 + 1)

bool Text::lineInfo(int line, int *start, int *len) {                  // 0x413ac0
    int n = (int)str.size(), seen = 0, i = 0;
    bool ok = true;
    for (;;) {
        if (line <= seen) {
            if (ok) {
                *start = i;
                *len = 0;
                for (; i < n; i++) {
                    if (i > -1 && str[i] == '\n') return true;
                    (*len)++;
                }
            }
            return ok;
        }
        if (i < n) { if (str[i] == '\n') seen++; }
        else ok = false;
        i++;
        if (!ok) return false;
    }
}

int Text::index(int line, int col) {                 // 0x413b50
    int n = (int)str.size();
    if (n == 0) return -1;
    int ln = 0, c = 0;
    for (int i = 0; i < n; i++) {
        if (ln == line) { bool hit = c == col; c++; if (hit) return i; }
        if (str[i] == '\n') { ln++; c = 0; }
    }
    return -1;
}

void Text::measure() {                               // 0x413110
    Measure m(this);
    const char *s = str.c_str();
    int n = (int)str.size(), nw, nh;
    if (n == 0) {
        TbSize z = m.extent(" ", 1);                 // 0x534f1c
        nw = z.cx; nh = z.cy;
    } else {
        int maxw = 0, maxh = 0, lines = 0, start = 0;
        for (int nl = (int)str.find('\n') + 1; nl != 0; nl = (int)str.find('\n', start) + 1) {   // 0x4240b0
            TbSize z = m.extent(s + start, nl - start - 1);
            lines++;
            start = nl;
            if (maxw < z.cx) maxw = z.cx;
            if (maxh < z.cy) maxh = z.cy;
            if (start >= n) break;
        }
        if (lines != 0) {                            // the last line, after the final newline
            TbSize z = m.extent(s + start, n - start);
            if (maxw < z.cx) maxw = z.cx;
            if (maxh < z.cy) maxh = z.cy;
            nw = maxw; nh = (lines + 1) * maxh;
        } else {
            TbSize z = m.extent(s, n);
            nw = z.cx; nh = z.cy;
        }
    }
    int dw = nw - w, dh = nh - h;
    w = nw; ow = nw; h = nh; oh = nh;                // +0x64 +0x3c, +0x68 +0x40
    if (nw != 0 && nh != 0) {
        x += -half(halign * dw); ox = x;             // the aligned edge stays where it was
        y += -half(valign * dh); oy = y;
    }
    if (layer) dirty();                              // 0x41e9e0 (Screen::sort + dirty)
}

int Text::maxLineWidth() {                           // 0x413320
    Measure m(this);
    int best = 0;
    int lines = lineCount();
    for (int i = 0; i < lines; i++) {
        int s, l, wd = 0;
        if (!str.empty() && lineInfo(i, &s, &l) && l > 0) wd = m.extent(str.c_str() + s, l).cx;
        if (best < wd) best = wd;
    }
    return best;
}

int Text::totalHeight() {                            // 0x413440
    Measure m(this);
    TbSize z = m.extent(" ", 1);
    return z.cy * lineCount();
}

int Text::lineCenterY(int line) {                    // 0x4134b0: the y of the middle of a line
    int L = lineCount();
    int cy;
    { Measure m(this); cy = m.extent(" ", 1).cy; }
    if (valign == -1) return cy * line - half(h + 1) + half(cy + 1) + y;
    if (valign == 0) return (line - L) * cy + y + ((L & 1) ? half(cy + 1) : 0);
    if (valign == 1) return half(h + 1) - (L - line) * cy + half(cy + 1) + y;
    return 0;
}

int Text::hitLine(int py) {                          // 0x4135d0
    int L = lineCount();
    int cy;
    { Measure m(this); cy = m.extent(" ", 1).cy; }
    int top = 0;
    if (valign == -1) top = y - half(h + 1);
    else if (valign == 0) top = y - half(cy * L + 1);
    else if (valign == 1) top = half(h + 1) - cy * L + y;
    for (int i = 0; i < L; i++, top += cy)
        if (top <= py && py <= top + cy) return i;
    return -1;
}

int Text::hitCol(int line, int px) {                 // 0x4136d0
    int s, l;
    if (!lineInfo(line, &s, &l)) return -1;
    if (l < 0) l = (int)str.size() - s;
    Measure m(this);
    int lw = l > 0 ? m.extent(str.c_str() + s, l).cx : 0;
    int left = 0;
    if (halign == -1) left = x - half(w + 1);
    else if (halign == 0) left = x - half(lw + 1);
    else if (halign == 1) left = half(w + 1) + x - lw;
    if (px < left) return -1;
    if (l <= 0) return 0;
    std::vector<int> dx((size_t)l + 1, 0);
    TbSize z = { 0, 0 };
    tb_GetTextExtentExPointA(m.dc, str.c_str() + s, l, 0, nullptr, dx.data(), &z);
    int col = 0;
    while (col < l && dx[col] < px - left) col++;    // the first character whose right edge is past the click
    return col;
}

bool Text::hitTest(int px, int py, int *line, int *col) {   // 0x413a80
    int ln = hitLine(py);
    *line = ln;
    if (ln < 0) return false;
    int c = hitCol(ln, px);
    *col = c;
    return c >= 0;
}

bool Text::charPos(int line, int col, int *outx, int *outy) {   // 0x4138a0: x of the caret in (line, col), y of the line
    int s = 0, l = 0;
    lineInfo(line, &s, &l);
    *outy = lineCenterY(line);
    Measure m(this);
    if (l < 0) l = (int)str.size() - s;
    int before = col > 0 ? m.extent(str.c_str() + s, std::min(col, l)).cx : 0;
    int whole = l > 0 ? m.extent(str.c_str() + s, l).cx : 0;
    if (halign == -1) *outx = x - half(w + 1) + before;
    else if (halign == 0) *outx = x - half(whole + 1) + before;
    else if (halign == 1) *outx = half(w + 1) + x - whole + before;
    else *outx = 0;
    return true;
}

// ---------------------------------------------------------------- Rectangle
Rectangle::Rectangle(int x_, int y_, int z_, int w_, int h_, bool vis, uint32_t col)   // 0x4011b0 (the caret)
    : Graphic(x_, y_, z_, w_, h_, 0, vis, col, 0, 0, 0, -1, 0xffff00ffu) {}

// 0x412db0: clipped to the canvas (cw x ch, stride pitch); a < 255 blends with the pixel's own channels.
void fill_rect565(int x, int y, int w, int h, int c0, int c1, int c2, int a, int cw, int ch, uint16_t *pixels, int pitch) {
    int right = x + w, bottom = y + h;
    if (x < 0) { x = 0; w = right; }
    if (cw <= right) w += cw - right;
    if (w <= 0) return;
    if (y < 0) { y = 0; h = bottom; }
    if (ch < bottom) h += ch - bottom;
    if (h <= 0) return;
    a &= 0xff; c0 &= 0xff; c1 &= 0xff; c2 &= 0xff;
    // c2 is the red (the low byte of the 0x00BBGGRR word), c1 green, c0 blue
    uint16_t full = (uint16_t)(((c2 >> 3) << 11) | ((c1 >> 2) << 5) | (c0 >> 3));
    uint16_t *row = pixels + (size_t)y * pitch + x;
    if (a == 0xff) {
        for (int r = 0; r < h; r++, row += pitch)
            for (int c = 0; c < w; c++) row[c] = full;
        return;
    }
    // the exe premultiplies each channel as (c * a) & 0xff, converts, then multiplies by a again (a quirk kept)
    int pc2 = (c2 * a) & 0xff, pc1 = (c1 * a) & 0xff, pc0 = (c0 * a) & 0xff;
    uint32_t pr = (uint32_t)((pc2 >> 3) << 11), pg = (uint32_t)((pc1 >> 2) << 5), pb = (uint32_t)(pc0 >> 3);
    uint32_t ia = 0xff - a;
    for (int r = 0; r < h; r++, row += pitch)
        for (int c = 0; c < w; c++) {
            uint32_t d = row[c];
            row[c] = (uint16_t)(((((d & 0x1f) * ia + pb * a) >> 8) & 0x1f) | ((((d & 0x7e0) * ia + pg * a) >> 8) & 0x7e0) |
                                ((((d & 0xf800) * ia + pr * a) >> 8) & 0xf800));
        }
}

void Rectangle::draw() {                             // 0x412870
    Canvas *cv = current_canvas();                   // 0x41eaa0
    int cw = cv->w, ch = cv->h;
    uint8_t a = (uint8_t)(color >> 24);              // +0x73
    if (a == 0 || !cv->pixels) return;
    int c0 = color & 0xff, c1 = (color >> 8) & 0xff, c2 = (color >> 16) & 0xff;     // +0x70, +0x71, +0x72
    int f = f78;
    uint16_t *px = cv->pixels; int pt = cv->pitch;
    auto fill = [&](int fx, int fy, int fw, int fh) { fill_rect565(fx, fy, fw, fh, c0, c1, c2, a, cw, ch, px, pt); };
    int L = -((w & 1) + w / 2) + x + cw / 2;         // left edge (the exe's odd-size adjustment)
    int T = -((h & 1) + h / 2) + y + ch / 2;
    if (f == 0) { fill(L, T, w, h); return; }
    if (f > 0) {                                     // a frame inside
        fill(L, T, w, f);
        fill(L, h / 2 + ch / 2 - f + y, w, f);
        fill(L, ch / 2 + f + (-(h & 1) - h / 2) + y, f, h - 2 * f);
        fill(cw / 2 + w / 2 - f + x, ch / 2 + f + (-(h & 1) - h / 2) + y, f, h - 2 * f);
        return;
    }
    int t = -f;                                      // a frame outside; f78 is flipped while drawing, then restored
    f78 = t;
    fill(L - t, T - t, w + 2 * t, t);
    fill(L - t, h / 2 + ch / 2 + y, w + 2 * t, t);
    fill(L - t, T, t, h);
    fill(cw / 2 + w / 2 + x, T, t, h);
    f78 = -t;
}

// ---------------------------------------------------------------- Joystick
static Node &joystick_root() { static Node n; return n; }      // 0x5a6c50
Node *Joystick::root() { return &joystick_root(); }            // 0x48ed60

// ---------------------------------------------------------------- Impl@TextField
TextFieldImpl::TextFieldImpl(TextField *o, Graphic *c)         // 0x401740
    : Task(nullptr), owner(o), mouse(), kbd(), text(), caret(c) {
    text.font = &default_font();
    text.f_a8 = default_font().f38;
    text.valign = -1; text.halign = -1;              // +0x1dc +0x1e0
    mouse.setClient(this);                           // 0x41f290
    kbd.setClient(this);
    text.detach();                                   // 0x412200
    caret->visible = false;
    line = -1; col = -1;                             // +0x20c +0x210 start at -1 (0x1dc.. in the decompile), then reset by setText
    line = 0; col = 0;                               // port: the exe leaves these to the first setText / click
}
TextFieldImpl::~TextFieldImpl() {                    // 0x401940
    if (ownsCaret && caret) delete caret;
    kbd.deactivate();
    mouse.deactivate();
}

void TextFieldImpl::run() {                          // 0x402670: toggles the caret every `blink` ms
    wait(1);
    for (;;) {
        caret->visible = !caret->visible;            // port: the exe's other branch (0x403d60) is for a subclassed caret
        wait((uint32_t)blink);
    }
}

void TextFieldImpl::notify(Service *s) {             // 0x402720
    if (s == &mouse && g_mouse.ldown) click();
    if (s == &kbd) {
        key();
        if (notifyClient) {                          // 0x41f1f0: tell this service's own client
            if (client) { client->notify(this); client->notify2(this); }
            else vision_log("Null client for service %s.", name());
        }
    }
}

void TextFieldImpl::eraseAt(int idx) {               // 0x424250
    if (idx >= 0 && idx < (int)text.str.size()) { text.str.erase((size_t)idx, 1); text.dirty(); }
}
bool TextFieldImpl::moveLeft() {                     // 0x401ff0
    int old = col;
    col--;
    if (col < 0) {
        if (line < 1) { col = old; return false; }
        line--;
        int s, l;
        text.lineInfo(line, &s, &l);
        col = l;
    }
    anchor = col;
    return true;
}
bool TextFieldImpl::atEnd() {                        // 0x401f40: the caret is at the end of its line
    int idx = text.index(line, col);
    if (idx != -1) {
        int s, l;
        text.lineInfo(line, &s, &l);
        if (col < l) return false;
    }
    return true;
}
bool TextFieldImpl::moveRight() {                    // 0x402050
    if (line + 1 == text.lineCount()) {
        if (text.index(line, col) == -1) return false;
        int s, l;
        text.lineInfo(line, &s, &l);
        if (l <= col) return false;
    }
    int idx = text.index(line, col);
    if (idx != -1) {
        int s, l;
        text.lineInfo(line, &s, &l);
        if (col < l) { col++; anchor = col; return true; }
    }
    if (line + 1 == text.lineCount()) return false;
    line++; col = 0; anchor = col;
    return true;
}
void TextFieldImpl::clampCol() {                     // 0x401fb0
    int s, l;
    text.lineInfo(line, &s, &l);
    if (l < col) col = l;
}

// 0x402160: after typing (or deleting), keep the text inside the box. port: simplified; the exe also breaks a
// too-wide line at its last space and replays the characters after it.
void TextFieldImpl::reflow(bool typed) {
    (void)typed;
    if (text.maxLineWidth() <= owner->w) return;
    if (multiline && text.totalHeight() < owner->h) {
        // break the line at the last space before the caret, else at the caret
        int idx = text.index(line, col);
        int brk = idx < 0 ? (int)text.str.size() : idx;
        size_t sp = text.str.rfind(' ', brk > 0 ? (size_t)brk - 1 : 0);
        int s, l;
        text.lineInfo(line, &s, &l);
        if (sp != std::string::npos && (int)sp >= s) {
            text.str[sp] = '\n';
            line++; col = brk - (int)sp - 1; anchor = col;
        } else {
            text.str.insert((size_t)brk, 1, '\n');
            line++; col = 0; anchor = 0;
        }
        text.dirty();
        return;
    }
    // it does not fit and cannot wrap: the typed character is taken back
    if (moveLeft()) eraseAt(text.index(line, col));
    reflow(false);
}

bool TextFieldImpl::accepts(int c) {                 // 0x4024a0
    if (c == 0xd || c == 10) return multiline;
    if (c == 0x20) return true;
    bool digit = c >= '0' && c <= '9', alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    switch (filter) {
    case 0: return true;
    case 1: return digit;
    case 2: return digit || alpha;
    case 3: return digit || alpha || c == '*';
    default: filter = 0; return true;
    }
}

void TextFieldImpl::insert(int c) {                  // 0x402560
    if (!accepts(c)) return;
    if (maxLen != -1 && maxLen <= (int)text.str.size()) return;
    if (c == 0xd) c = 10;
    if ((!multiline || text.totalHeight() >= owner->h) && c == 10) return;
    int idx = text.index(line, col);
    if (idx == -1) text.str.push_back((char)c);
    else text.str.insert((size_t)idx, 1, (char)c);
    text.dirty();
    if (c == 10) { col = 0; line++; anchor = col; return; }
    col++;
    reflow(true);
    anchor = col;
}

void TextFieldImpl::click() {                        // 0x401a30
    if (!owner->hit(g_mouse.x, g_mouse.y)) {         // a click outside gives the focus away
        if (focus) {
            kbd.deactivate();
            owner->impl->deactivate();
            owner->impl->caret->visible = false;
        }
        return;
    }
    if (!focus && !kbd.active()) return;
    int n = text.lineCount();
    int L = n < 1 ? 0 : n - 1, s = 0, l = 0;
    if (!text.lineInfo(L, &s, &l)) l = 0;
    int C = l < 1 ? 0 : l;
    line = L; col = C; anchor = C;
    int hl, hc;
    if (text.hitTest(g_mouse.x, g_mouse.y, &hl, &hc)) { line = hl; col = hc; }
    if (line >= text.lineCount()) line = text.lineCount() - 1;
    if (line < 0) line = 0;
    text.lineInfo(line, &s, &l);
    if (col > l) col = l;
    anchor = col;
    prevLen = (int)text.str.size();
    caret->load();
    caret->visible = true;
    kbd.activate();
    activate();
}

void TextFieldImpl::key() {                          // 0x401c10
    if (!g_key.chr) {
        if (!g_key.down) return;
        switch (g_key.vk) {
        case 0x26:                                   // up
            if (line != 0) { line--; col = anchor; clampCol(); }
            break;
        case 0x28:                                   // down
            if (line + 1 != text.lineCount()) { line++; col = anchor; clampCol(); }
            break;
        case 0x27: moveRight(); break;
        case 0x25: moveLeft(); break;
        case 8:                                      // backspace
            if (moveLeft()) { eraseAt(text.index(line, col)); reflow(false); }
            break;
        case 0x2e:                                   // delete
            if (!(line + 1 == text.lineCount() && atEnd())) { eraseAt(text.index(line, col)); reflow(false); }
            break;
        case 0x23: {                                 // end
            int s, l;
            text.lineInfo(line, &s, &l);
            col = l; anchor = col;
            break;
        }
        case 0x24: col = 0; anchor = 0; break;       // home
        }
        return;
    }
    int c = (int)g_key.vk;
    if ((c >= 0x20 && c < 0x7f) || c >= 0xa0 || (c == 0xd && !owner->enter_ends)) insert(c);   // isprint / Enter
}

// ---------------------------------------------------------------- TextField
TextField::TextField()                                // 0x4011b0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {
    impl = new TextFieldImpl(this, new Rectangle(0, 0, 0, 2, 25, true, 0xff000000u));
    impl->prevLen = (int)impl->text.str.size();      // 0x424540
    impl->prevLen = 0;
}
TextField::~TextField() { delete impl; }              // 0x401320
void TextField::hide() {                              // 0x4013a0
    impl->kbd.deactivate();
    impl->deactivate();
    impl->caret->visible = false;
}
void TextField::setText(const char *s) {              // 0x4013e0
    impl->line = 0; impl->col = 0;
    impl->text.str.clear();
    for (; s && *s; s++) impl->insert((unsigned char)*s);
    impl->prevLen = (int)impl->text.str.size();
}
bool TextField::focused() { return impl->kbd.active(); }    // 0x401470
const char *TextField::getText() { return impl->text.str.empty() ? "" : impl->text.str.c_str(); }   // 0x401490
void TextField::load() {                              // 0x4014b0
    setHit(hitmode, false);
    impl->text.load();
    impl->active_ = true;
    impl->focus = true;
    impl->kbd.deactivate();
    impl->deactivate();
    impl->caret->visible = false;
    impl->mouse.activate();
}
void TextField::unload() {                            // 0x401540
    Graphic::unload();
    impl->kbd.deactivate();
    impl->deactivate();
    impl->caret->visible = false;
    impl->caret->unload();
    impl->focus = false;
    impl->active_ = false;
}
void TextField::draw() {                              // 0x4015b0
    TextFieldImpl *i = impl;
    int cx, cy;
    if (!i->text.charPos(i->line, i->col, &cx, &cy)) { cy = i->text.y; cx = i->text.x - w / 2; }
    i->caret->x = cx; i->caret->y = cy;
    i->text.measure();
    i->text.x = i->centered ? i->text.maxLineWidth() / 2 + (x - w / 2) : x;
    i->text.y = i->multiline ? i->text.totalHeight() / 2 + (y - h / 2) : y;
    i->text.color = color;
    // port: the exe's Text::draw finds the DC of a mode-2 canvas; a TextField is a mode-1 graphic, so switch here
    canvas_set_mode(2);
    i->text.draw();
    canvas_set_mode(1);
    if (i->caret->visible) {
        i->caret->color = color;
        i->caret->draw();
    }
}

}  // namespace Vision
