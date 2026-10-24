// GDI shim (see gdi.h). port: bundled Liberation Sans + stb_truetype instead of Windows' fonts.
#include "gdi.h"
#include "tb.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "third_party/stb_truetype.h"

// ---- the bundled fonts (SIL OFL 1.1; platform/fonts/), linked into the binary with .incbin
#ifndef TB_FONT_DIR
#error "TB_FONT_DIR must be the absolute path of platform/fonts (set in CMakeLists.txt)"
#endif
#define TB_INCBIN(sym, file)                                                              \
    __asm__(".section .rodata\n.balign 16\n.global " #sym "\n" #sym ":\n.incbin \"" TB_FONT_DIR "/" file "\"\n" \
            ".global " #sym "_end\n" #sym "_end:\n.byte 0\n.previous\n");                  \
    extern "C" const uint8_t sym[]; extern "C" const uint8_t sym##_end[];
TB_INCBIN(tb_font_regular, "LiberationSans-Regular.ttf")
TB_INCBIN(tb_font_bold, "LiberationSans-Bold.ttf")
TB_INCBIN(tb_font_italic, "LiberationSans-Italic.ttf")
TB_INCBIN(tb_font_bolditalic, "LiberationSans-BoldItalic.ttf")

namespace {

struct Glyph { int adv = 0, w = 0, h = 0, xoff = 0, yoff = 0; std::vector<uint8_t> cov; };

struct Face {
    stbtt_fontinfo info;
    bool ok = false;
};

Face g_faces[4];   // regular, bold, italic, bold italic

Face &face(bool bold, bool italic) {
    static const uint8_t *data[4] = { tb_font_regular, tb_font_bold, tb_font_italic, tb_font_bolditalic };
    int i = (bold ? 1 : 0) + (italic ? 2 : 0);
    Face &f = g_faces[i];
    if (!f.ok) {
        f.ok = stbtt_InitFont(&f.info, data[i], stbtt_GetFontOffsetForIndex(data[i], 0)) != 0;
        if (!f.ok) tb_log("gdi: bundled font %d failed to load", i);
    }
    return f;
}

// CP1252 0x80..0x9F -> Unicode (the game's text is Windows ANSI)
const uint16_t kCp1252[32] = {
    0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x8D, 0x017D, 0x8F,
    0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D, 0x017E, 0x0178 };
inline int codepoint(unsigned char c) { return c >= 0x80 && c < 0xA0 ? kCp1252[c - 0x80] : c; }

}  // namespace

struct TbGdiObj {
    enum Kind { FONT, PEN, BRUSH } kind = FONT;
    bool stock = false;
    // font
    TbLogFont lf;
    Face *face = nullptr;
    float scale = 1;
    int ascent = 0, descent = 0, height = 0, avgw = 0, internal = 0;
    std::unordered_map<int, Glyph> cache;
    // pen / brush
    int style = 0, width = 1;
    TbCOLORREF color = 0;

    const Glyph &glyph(int cp) {
        auto it = cache.find(cp);
        if (it != cache.end()) return it->second;
        Glyph &g = cache[cp];
        int adv = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&face->info, cp, &adv, &lsb);
        g.adv = (int)floorf(adv * scale + 0.5f);
        int w = 0, h = 0, xo = 0, yo = 0;
        unsigned char *bm = stbtt_GetCodepointBitmap(&face->info, scale, scale, cp, &w, &h, &xo, &yo);
        if (bm) {
            g.w = w; g.h = h; g.xoff = xo; g.yoff = yo;
            g.cov.assign(bm, bm + (size_t)w * h);
            stbtt_FreeBitmap(bm, nullptr);
        }
        return g;
    }
};

struct TbDC {
    TbSurfaceBits bits;
    bool hasBits = false;
    TbCOLORREF textColor = 0, bkColor = 0xFFFFFF;
    int bkMode = 2;
    int orgX = 0, orgY = 0;
    TbGdiObj *font, *pen, *brush;
    // pixel format helpers
    int rs = 0, gs = 0, bs = 0, rb = 0, gb = 0, bb = 0;
};

namespace {

TbGdiObj g_stock[18];
bool g_stock_init = false;

int ctz32(uint32_t v) { int n = 0; if (!v) return 0; while (!(v & 1)) { v >>= 1; n++; } return n; }
int pop32(uint32_t v) { int n = 0; while (v) { n += v & 1; v >>= 1; } return n; }

void layoutFont(TbGdiObj *f) {
    Face &fc = face(f->lf.lfWeight >= 600, f->lf.lfItalic != 0);
    f->face = &fc;
    int h = f->lf.lfHeight;
    // lfHeight < 0: character (em) height in pixels; > 0: cell height; 0: a default
    if (h == 0) h = -12;
    f->scale = h < 0 ? stbtt_ScaleForMappingEmToPixels(&fc.info, (float)-h) : stbtt_ScaleForPixelHeight(&fc.info, (float)h);
    int a, d, lg;
    stbtt_GetFontVMetrics(&fc.info, &a, &d, &lg);
    f->ascent = (int)ceilf(a * f->scale);
    f->descent = (int)ceilf(-d * f->scale);
    f->height = f->ascent + f->descent;
    f->internal = f->height - (h < 0 ? -h : (int)floorf((a - d) * f->scale + 0.5f) - 0);
    if (f->internal < 0) f->internal = 0;
    f->avgw = f->glyph('x').adv;
}

TbGdiObj *makeFont(const TbLogFont &lf) {
    TbGdiObj *o = new TbGdiObj;
    o->kind = TbGdiObj::FONT;
    o->lf = lf;
    layoutFont(o);
    return o;
}

void initStock() {
    if (g_stock_init) return;
    g_stock_init = true;
    static const TbCOLORREF grays[5] = { 0xFFFFFF, 0xC0C0C0, 0x808080, 0x404040, 0x000000 };
    for (int i = 0; i < 5; i++) { g_stock[i].kind = TbGdiObj::BRUSH; g_stock[i].stock = true; g_stock[i].color = grays[i]; }
    g_stock[5].kind = TbGdiObj::BRUSH; g_stock[5].stock = true; g_stock[5].style = 1;   // NULL_BRUSH
    g_stock[6].kind = TbGdiObj::PEN; g_stock[6].stock = true; g_stock[6].color = 0xFFFFFF;
    g_stock[7].kind = TbGdiObj::PEN; g_stock[7].stock = true; g_stock[7].color = 0;
    g_stock[8].kind = TbGdiObj::PEN; g_stock[8].stock = true; g_stock[8].style = 5;      // NULL_PEN
    for (int i = 9; i < 18; i++) {
        if (i == 15) continue;
        TbLogFont lf; memset(&lf, 0, sizeof lf);
        lf.lfHeight = -13; lf.lfWeight = 400; lf.lfCharSet = 0;
        strncpy(lf.lfFaceName, "Arial", sizeof lf.lfFaceName - 1);
        g_stock[i].kind = TbGdiObj::FONT; g_stock[i].stock = true; g_stock[i].lf = lf;
        layoutFont(&g_stock[i]);
    }
}

TbDC g_screen_dc;
bool g_screen_init = false;

void dcDefaults(TbDC *dc) {
    initStock();
    dc->font = &g_stock[TB_SYSTEM_FONT];
    dc->pen = &g_stock[TB_BLACK_PEN];
    dc->brush = &g_stock[TB_WHITE_BRUSH];
}

inline void unpack(const TbDC *dc, uint32_t p, int &r, int &g, int &b) {
    uint32_t rv = (p & dc->bits.rmask) >> dc->rs, gv = (p & dc->bits.gmask) >> dc->gs, bv = (p & dc->bits.bmask) >> dc->bs;
    r = dc->rb >= 8 ? (int)(rv >> (dc->rb - 8)) : (int)((rv << (8 - dc->rb)) | (rv >> (2 * dc->rb - 8 < 0 ? 0 : 2 * dc->rb - 8)));
    g = dc->gb >= 8 ? (int)(gv >> (dc->gb - 8)) : (int)((gv << (8 - dc->gb)) | (gv >> (2 * dc->gb - 8 < 0 ? 0 : 2 * dc->gb - 8)));
    b = dc->bb >= 8 ? (int)(bv >> (dc->bb - 8)) : (int)((bv << (8 - dc->bb)) | (bv >> (2 * dc->bb - 8 < 0 ? 0 : 2 * dc->bb - 8)));
}
inline uint32_t pack(const TbDC *dc, int r, int g, int b) {
    return ((uint32_t)(r >> (8 - dc->rb)) << dc->rs) | ((uint32_t)(g >> (8 - dc->gb)) << dc->gs) | ((uint32_t)(b >> (8 - dc->bb)) << dc->bs);
}
inline uint32_t readPx(const TbDC *dc, int x, int y) {
    const uint8_t *row = (const uint8_t *)dc->bits.pixels + (size_t)y * dc->bits.pitch;
    return dc->bits.bpp == 16 ? ((const uint16_t *)row)[x] : ((const uint32_t *)row)[x];
}
inline void writePx(TbDC *dc, int x, int y, uint32_t p) {
    uint8_t *row = (uint8_t *)dc->bits.pixels + (size_t)y * dc->bits.pitch;
    if (dc->bits.bpp == 16) ((uint16_t *)row)[x] = (uint16_t)p; else ((uint32_t *)row)[x] = p;
}

struct Clip { int l, t, r, b; };   // device pixels, right/bottom exclusive

inline Clip surfaceClip(const TbDC *dc) { return { 0, 0, dc->bits.width, dc->bits.height }; }

void fillRect(TbDC *dc, int l, int t, int r, int b, TbCOLORREF c, Clip clip) {
    if (!dc->hasBits) return;
    l = std::max(l, clip.l); t = std::max(t, clip.t); r = std::min(r, clip.r); b = std::min(b, clip.b);
    if (l >= r || t >= b) return;
    uint32_t px = pack(dc, c & 255, (c >> 8) & 255, (c >> 16) & 255);
    for (int y = t; y < b; y++) for (int x = l; x < r; x++) writePx(dc, x, y, px);
}

void blendGlyph(TbDC *dc, const Glyph &g, int gx, int gy, TbCOLORREF c, Clip clip) {
    if (!dc->hasBits) return;
    int cr = c & 255, cg = (c >> 8) & 255, cb = (c >> 16) & 255;
    for (int j = 0; j < g.h; j++) {
        int y = gy + j;
        if (y < clip.t || y >= clip.b) continue;
        for (int i = 0; i < g.w; i++) {
            int x = gx + i;
            if (x < clip.l || x >= clip.r) continue;
            int a = g.cov[(size_t)j * g.w + i];
            if (!a) continue;
            if (a >= 250) { writePx(dc, x, y, pack(dc, cr, cg, cb)); continue; }
            int r, gg, b;
            unpack(dc, readPx(dc, x, y), r, gg, b);
            r += ((cr - r) * a + 127) / 255;
            gg += ((cg - gg) * a + 127) / 255;
            b += ((cb - b) * a + 127) / 255;
            writePx(dc, x, y, pack(dc, r, gg, b));
        }
    }
}

int stringWidth(TbGdiObj *f, const char *s, int len) {
    int w = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\t') { w += 8 * f->glyph(' ').adv; continue; }
        if (c < 32) continue;
        w += f->glyph(codepoint(c)).adv;
    }
    return w;
}

// Draws one line with its top-left at device (x, y); returns its width.
int drawLine(TbDC *dc, int x, int y, const char *s, int len, Clip clip) {
    TbGdiObj *f = dc->font;
    int w = stringWidth(f, s, len);
    if (dc->bkMode == 2) fillRect(dc, x, y, x + w, y + f->height, dc->bkColor, clip);
    int pen = x;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\t') { pen += 8 * f->glyph(' ').adv; continue; }
        if (c < 32) continue;
        const Glyph &g = f->glyph(codepoint(c));
        blendGlyph(dc, g, pen + g.xoff, y + f->ascent + g.yoff, dc->textColor, clip);
        pen += g.adv;
    }
    if (f->lf.lfUnderline) fillRect(dc, x, y + f->ascent + 1, x + w, y + f->ascent + 2, dc->textColor, clip);
    if (f->lf.lfStrikeOut) fillRect(dc, x, y + f->ascent * 2 / 3, x + w, y + f->ascent * 2 / 3 + 1, dc->textColor, clip);
    return w;
}

}  // namespace

// ---------------------------------------------------------------------------------------------------------------

TbHDC tb_gdi_surface_dc(const TbSurfaceBits &bits) {
    TbDC *dc = new TbDC;
    dc->bits = bits;
    dc->hasBits = bits.pixels && (bits.bpp == 16 || bits.bpp == 32);
    dc->rs = ctz32(bits.rmask); dc->gs = ctz32(bits.gmask); dc->bs = ctz32(bits.bmask);
    dc->rb = std::max(1, pop32(bits.rmask)); dc->gb = std::max(1, pop32(bits.gmask)); dc->bb = std::max(1, pop32(bits.bmask));
    dcDefaults(dc);
    return dc;
}
void tb_gdi_release_surface_dc(TbHDC dc) { delete dc; }

TbHDC tb_GetDC(void *) {
    if (!g_screen_init) {
        g_screen_init = true;
        memset(&g_screen_dc.bits, 0, sizeof g_screen_dc.bits);
        g_screen_dc.hasBits = false;
        dcDefaults(&g_screen_dc);
    }
    // each GetDC starts with the default state
    g_screen_dc.textColor = 0; g_screen_dc.bkColor = 0xFFFFFF; g_screen_dc.bkMode = 2; g_screen_dc.orgX = g_screen_dc.orgY = 0;
    dcDefaults(&g_screen_dc);
    return &g_screen_dc;
}
int tb_ReleaseDC(void *, TbHDC) { return 1; }

TbHGDIOBJ tb_CreateFontIndirectA(const TbLogFont *lf) { return lf ? makeFont(*lf) : nullptr; }

TbHGDIOBJ tb_CreateFontA(int h, int w, int esc, int orient, int weight, uint32_t italic, uint32_t underline,
                         uint32_t strike, uint32_t charset, uint32_t outprec, uint32_t clipprec, uint32_t quality,
                         uint32_t pitchfam, const char *face) {
    TbLogFont lf; memset(&lf, 0, sizeof lf);
    lf.lfHeight = h; lf.lfWidth = w; lf.lfEscapement = esc; lf.lfOrientation = orient;
    lf.lfWeight = weight ? weight : 400;
    lf.lfItalic = italic != 0; lf.lfUnderline = underline != 0; lf.lfStrikeOut = strike != 0;
    lf.lfCharSet = (uint8_t)charset; lf.lfOutPrecision = (uint8_t)outprec; lf.lfClipPrecision = (uint8_t)clipprec;
    lf.lfQuality = (uint8_t)quality; lf.lfPitchAndFamily = (uint8_t)pitchfam;
    if (face) strncpy(lf.lfFaceName, face, sizeof lf.lfFaceName - 1);
    return makeFont(lf);
}

TbHGDIOBJ tb_CreatePen(int style, int width, TbCOLORREF color) {
    TbGdiObj *o = new TbGdiObj; o->kind = TbGdiObj::PEN; o->style = style; o->width = width; o->color = color; return o;
}
TbHGDIOBJ tb_CreateBrushIndirect(const TbLogBrush *lb) {
    TbGdiObj *o = new TbGdiObj; o->kind = TbGdiObj::BRUSH; o->style = lb ? (int)lb->lbStyle : 0; o->color = lb ? lb->lbColor : 0; return o;
}
TbHGDIOBJ tb_GetStockObject(int i) {
    initStock();
    if (i < 0 || i > 17 || i == 15) return nullptr;
    if (i == 9) i = TB_SYSTEM_FONT;
    return &g_stock[i];
}
bool tb_DeleteObject(TbHGDIOBJ o) {
    if (!o) return false;
    if (o->stock) return true;
    delete o;
    return true;
}

TbHGDIOBJ tb_SelectObject(TbHDC dc, TbHGDIOBJ o) {
    if (!dc || !o) return nullptr;
    TbGdiObj *old = nullptr;
    switch (o->kind) {
    case TbGdiObj::FONT: old = dc->font; dc->font = o; break;
    case TbGdiObj::PEN: old = dc->pen; dc->pen = o; break;
    case TbGdiObj::BRUSH: old = dc->brush; dc->brush = o; break;
    }
    return old;
}

TbCOLORREF tb_SetTextColor(TbHDC dc, TbCOLORREF c) { TbCOLORREF o = dc->textColor; dc->textColor = c & 0xFFFFFF; return o; }
int tb_SetBkMode(TbHDC dc, int m) { int o = dc->bkMode; dc->bkMode = m; return o; }
bool tb_SetViewportOrgEx(TbHDC dc, int x, int y, TbPoint *old) {
    if (old) { old->x = dc->orgX; old->y = dc->orgY; }
    dc->orgX = x; dc->orgY = y;
    return true;
}
int tb_GetDeviceCaps(TbHDC dc, int i) {
    switch (i) {
    case 88: case 90: return 96;                        // LOGPIXELSX / Y
    case 8: return dc && dc->hasBits ? dc->bits.width : 640;     // HORZRES
    case 10: return dc && dc->hasBits ? dc->bits.height : 480;   // VERTRES
    case 12: return dc && dc->hasBits ? dc->bits.bpp : 16;       // BITSPIXEL
    case 14: return 1;                                  // PLANES
    default: return 0;
    }
}

bool tb_TextOutA(TbHDC dc, int x, int y, const char *s, int len) {
    if (!dc || !s) return false;
    if (len < 0) len = (int)strlen(s);
    if (dc->hasBits) drawLine(dc, x + dc->orgX, y + dc->orgY, s, len, surfaceClip(dc));
    return true;
}

int tb_DrawTextA(TbHDC dc, const char *s, int len, TbRect *rc, uint32_t flags) {
    if (!dc || !s || !rc) return 0;
    if (len < 0) len = (int)strlen(s);
    TbGdiObj *f = dc->font;
    const bool single = flags & 0x20, wrap = (flags & 0x10) && !single;
    const int rw = rc->right - rc->left;
    // split into lines
    struct Line { int off, len; };
    std::vector<Line> lines;
    int start = 0;
    auto addWrapped = [&](int off, int n) {
        if (!wrap) { lines.push_back({ off, n }); return; }
        int ls = off, end = off + n;
        while (ls < end) {
            int w = 0, brk = -1, i = ls;
            for (; i < end; i++) {
                unsigned char c = (unsigned char)s[i];
                int cw = c == '\t' ? 8 * f->glyph(' ').adv : (c < 32 ? 0 : f->glyph(codepoint(c)).adv);
                if (w + cw > rw && i > ls) break;
                w += cw;
                if (c == ' ') brk = i;
            }
            if (i >= end) { lines.push_back({ ls, end - ls }); break; }
            if (brk >= ls) { lines.push_back({ ls, brk - ls }); ls = brk + 1; }
            else { lines.push_back({ ls, i - ls }); ls = i; }
        }
        if (ls == end && n == 0) lines.push_back({ off, 0 });
    };
    if (single) {
        std::string t;
        for (int i = 0; i < len; i++) if (s[i] != '\n' && s[i] != '\r') t += s[i];
        // single line: draw as one string (stored in a temp below)
        int w = stringWidth(f, t.data(), (int)t.size());
        int th = f->height;
        if (flags & 0x400) { rc->right = rc->left + w; rc->bottom = rc->top + th; return th; }
        int x = rc->left, y = rc->top;
        if (flags & 2) x = rc->right - w; else if (flags & 1) x = rc->left + (rw - w) / 2;
        int rh = rc->bottom - rc->top;
        if (flags & 8) y = rc->bottom - th; else if (flags & 4) y = rc->top + (rh - th) / 2;
        Clip clip = surfaceClip(dc);
        if (!(flags & 0x100)) {
            clip.l = std::max(clip.l, rc->left + dc->orgX); clip.t = std::max(clip.t, rc->top + dc->orgY);
            clip.r = std::min(clip.r, rc->right + dc->orgX); clip.b = std::min(clip.b, rc->bottom + dc->orgY);
        }
        if (dc->hasBits) drawLine(dc, x + dc->orgX, y + dc->orgY, t.data(), (int)t.size(), clip);
        return th;
    }
    for (int i = 0; i <= len; i++) {
        if (i == len || s[i] == '\n') {
            int n = i - start;
            if (n > 0 && s[start + n - 1] == '\r') n--;
            addWrapped(start, n);
            start = i + 1;
        }
    }
    if (len > 0 && s[len - 1] == '\n' && !lines.empty()) { /* trailing newline: Windows draws no extra empty line */ }
    int th = (int)lines.size() * f->height;
    if (flags & 0x400) {
        int maxw = 0;
        for (auto &l : lines) maxw = std::max(maxw, stringWidth(f, s + l.off, l.len));
        if (!wrap) rc->right = rc->left + maxw;
        rc->bottom = rc->top + th;
        return th;
    }
    Clip clip = surfaceClip(dc);
    if (!(flags & 0x100)) {
        clip.l = std::max(clip.l, rc->left + dc->orgX); clip.t = std::max(clip.t, rc->top + dc->orgY);
        clip.r = std::min(clip.r, rc->right + dc->orgX); clip.b = std::min(clip.b, rc->bottom + dc->orgY);
    }
    // port: Windows ignores DT_VCENTER / DT_BOTTOM without DT_SINGLELINE, so the block is top aligned
    int y = rc->top;
    for (auto &l : lines) {
        int w = stringWidth(f, s + l.off, l.len);
        int x = rc->left;
        if (flags & 2) x = rc->right - w; else if (flags & 1) x = rc->left + (rw - w) / 2;
        if (dc->hasBits) drawLine(dc, x + dc->orgX, y + dc->orgY, s + l.off, l.len, clip);
        y += f->height;
    }
    return th;
}

bool tb_Rectangle(TbHDC dc, int l, int t, int r, int b) {
    if (!dc) return false;
    l += dc->orgX; r += dc->orgX; t += dc->orgY; b += dc->orgY;
    Clip clip = surfaceClip(dc);
    TbGdiObj *pen = dc->pen, *br = dc->brush;
    int pw = pen->style == 5 ? 0 : std::max(1, pen->width);
    // PS_INSIDEFRAME and the default pen both keep the outline inside the rectangle
    if (br->style != 1) fillRect(dc, l + pw, t + pw, r - pw, b - pw, br->color, clip);
    if (pw) {
        fillRect(dc, l, t, r, t + pw, pen->color, clip);
        fillRect(dc, l, b - pw, r, b, pen->color, clip);
        fillRect(dc, l, t + pw, l + pw, b - pw, pen->color, clip);
        fillRect(dc, r - pw, t + pw, r, b - pw, pen->color, clip);
    }
    return true;
}

bool tb_GetTextExtentPoint32A(TbHDC dc, const char *s, int len, TbSize *sz) {
    if (!dc || !sz) return false;
    if (len < 0) len = s ? (int)strlen(s) : 0;
    sz->cx = s ? stringWidth(dc->font, s, len) : 0;
    sz->cy = dc->font->height;
    return true;
}

bool tb_GetTextExtentExPointA(TbHDC dc, const char *s, int len, int maxWidth, int *fit, int *dx, TbSize *sz) {
    if (!dc || !sz) return false;
    if (len < 0) len = s ? (int)strlen(s) : 0;
    int w = 0, n = 0;
    for (int i = 0; i < len; i++) {
        w += stringWidth(dc->font, s + i, 1);
        if (dx) dx[i] = w;
        if (maxWidth <= 0 || w <= maxWidth) n = i + 1;
    }
    if (fit) *fit = maxWidth <= 0 ? len : n;
    sz->cx = w;
    sz->cy = dc->font->height;
    return true;
}

int tb_EnumFontFamiliesExA(TbHDC, const TbLogFont *lf, TbFontEnumProc proc, intptr_t lparam, uint32_t) {
    if (!proc) return 0;
    TbLogFont out; memset(&out, 0, sizeof out);
    if (lf) out = *lf;
    out.lfHeight = out.lfHeight ? out.lfHeight : -12;
    if (!out.lfWeight) out.lfWeight = 400;
    TbGdiObj *f = makeFont(out);
    TbTextMetric tm; memset(&tm, 0, sizeof tm);
    tm.tmHeight = f->height; tm.tmAscent = f->ascent; tm.tmDescent = f->descent; tm.tmInternalLeading = f->internal;
    tm.tmAveCharWidth = f->avgw; tm.tmWeight = out.lfWeight; tm.tmFirstChar = 32; tm.tmLastChar = 255; tm.tmDefaultChar = '?';
    tm.tmBreakChar = ' '; tm.tmCharSet = out.lfCharSet;
    delete f;
    return proc(&out, &tm, 4 /* TRUETYPE_FONTTYPE */, lparam);
}
