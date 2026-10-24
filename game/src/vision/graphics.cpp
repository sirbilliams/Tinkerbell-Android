// Screen / Layer / Graphic / Image, the canvas and the sprite blitter, and the Screen service (lane A).
#include "engine.h"
#include "text.h"
#include "tb.h"
#include "../../../platform/ddraw.h"
#include <string.h>
#include <algorithm>

namespace Vision {

// ---------------------------------------------------------------- the DirectDraw objects (0x5a6794 ...)
static TbDirectDraw *g_dd = nullptr;     // 0x5a6794
static TbSurface *g_primary = nullptr;   // 0x5a6658
static TbSurface *g_back = nullptr;      // 0x5a67a0
static TbSurface *g_bg = nullptr;        // 0x5a679c: optional background copied in each frame
static Canvas g_screen_canvas;           // 0x5a73a0
static Canvas *g_canvas = &g_screen_canvas;   // 0x5a73e8
static int g_canvas_mode = 1;            // 0x532898: 1 = pixels locked, 2 = a GDI DC

Canvas *current_canvas() { return g_canvas; }

void lock_canvas() {                     // 0x40e390
    if (!g_canvas->surface) return;
    if (g_canvas_mode == 1) {
        int pitch = 0;
        g_canvas->surface->Lock(&g_canvas->pixels, &pitch);
        g_canvas->pitch = pitch / 2;
    }
    else if (g_canvas_mode == 2) canvas_dc_begin(g_canvas);   // GetDC on the surface, SetViewportOrgEx to the centre (text.cpp)
}
void unlock_canvas() {                   // 0x40e620
    if (!g_canvas->surface) return;
    if (g_canvas_mode == 1) { g_canvas->surface->Unlock(); g_canvas->pixels = nullptr; }
    else if (g_canvas_mode == 2) canvas_dc_end();            // ReleaseDC (vtbl +0x68)
    g_canvas_mode = 1;
}
// port (lane C): switch the canvas between pixels (1) and DC (2) from inside a draw, as 0x40e680 does between graphics
void canvas_set_mode(int m) {
    if (m == g_canvas_mode) return;
    unlock_canvas();
    g_canvas_mode = m;
    lock_canvas();
}

// 0x40dc90 Screen service init: DirectDraw, 16-bit mode, primary + back buffer
static bool screen_init() {
    if (tb_DirectDrawCreate(&g_dd) != 0) { vision_abort("Abort %s", "IID_IDirectDraw4"); return false; }
    if (g_dd->GetDisplayBpp() < 16) {
        vision_abort("Please use Display Settings in the Control Panel to change your color setting to 16 bit or higher.");
        return false;
    }
    g_dd->SetCooperativeLevel(nullptr, 0xa);     // windowed: DDSCL_NORMAL (8) | 2
    g_dd->CreateSurface(true, 0, 0, &g_primary);
    g_dd->CreateSurface(false, stage()->w, stage()->h, &g_back);
    g_screen_canvas.surface = g_back;
    g_screen_canvas.w = stage()->w;
    g_screen_canvas.h = stage()->h;
    uint16_t *p; int pitch;
    g_back->Lock(&p, &pitch);
    memset(p, 0, (size_t)pitch * g_back->height);
    g_back->Unlock();
    TbPixelFormat pf;                    // 0x40e0a0: the masks pick the converters; RGB565 = no conversion (0x5a67e4 = 0)
    tb_GetPixelFormat(g_back, &pf);
    return true;
}

static void screen_shutdown() {          // 0x40e2e0
    if (g_back) { g_back->Release(); g_back = nullptr; }
    if (g_primary) { g_primary->Release(); g_primary = nullptr; }
    if (g_dd) { g_dd->Release(); g_dd = nullptr; }
    g_screen_canvas.surface = nullptr;
}

// ---------------------------------------------------------------- Screen
static Node &screen_list() { static Node n; return n; }   // 0x5a6618 (first at 0x5a6628)

Screen::Screen(int x_, int y_, int depth_, int w_, int h_, bool vis, bool f79)   // 0x41eea0
    : Service(nullptr), x(x_), y(y_), depth(depth_), w(w_), h(h_), visible(vis), flag79(f79) {}
Screen::~Screen() {                      // 0x41ee10
    if (!tb_exiting()) Service::deactivate();
    for (size_t i = 0; i < layer_list.size(); i++) delete layer_list[i];
}
Node *Screen::root() { return &screen_list(); }
void Screen::activate() {                // 0x41edb0
    if (node) return;
    Node *after = nullptr;
    for (Node *n = screen_list().first; n && ((Screen *)n->obj)->depth < depth; n = n->next) after = n;
    node = screen_list().add(after, this);
    dirty();
}
void Screen::draw() {                    // 0x41eaf0
    if (!visible) return;
    for (size_t i = 0; i < layer_list.size(); i++) {
        Layer *l = layer_list[i];
        if (!l->visible) continue;
        for (Node *g = l->graphics.first; g; g = g->next) {
            Graphic *gr = (Graphic *)g->obj;
            // 0x40e680
            if (!gr->visible) continue;
            int m = gr->canvasMode();
            if (m != g_canvas_mode) { unlock_canvas(); g_canvas_mode = m; lock_canvas(); }
            gr->draw();
        }
    }
}
Layer *Screen::layer(int n) {            // 0x41ece0
    while ((int)layer_list.size() <= n) {
        Layer *l = new Layer(this);
        layers.add(nullptr, l);
        layer_list.push_back(l);
    }
    return layer_list[n];
}
void Screen::loadAll() {                 // 0x41ec40
    if (w == 0) w = stage()->w;
    if (h == 0) h = stage()->h;
    for (size_t i = 0; i < layer_list.size(); i++) {
        Layer *l = layer_list[i];
        for (Node *g = l->graphics.first; g; g = g->next) ((Graphic *)g->obj)->load();
        l->sort();
    }
    dirty();
}
void Screen::unloadAll() {               // 0x41ec00
    for (size_t i = 0; i < layer_list.size(); i++)
        for (Node *g = layer_list[i]->graphics.first; g; g = g->next) ((Graphic *)g->obj)->unload();
}
void Screen::clear() {                   // 0x41ebc0
    for (size_t i = 0; i < layer_list.size(); i++)
        while (layer_list[i]->graphics.first) ((Graphic *)layer_list[i]->graphics.first->obj)->detach();
}

// ---------------------------------------------------------------- Layer
Layer::Layer(Screen *s) : screen(s) {}
Layer::~Layer() {                        // 0x41ea10
    if (!tb_exiting())
        while (graphics.first) ((Graphic *)graphics.first->obj)->detach();
}
void Layer::add(Graphic *g) {            // 0x41e930
    if (g->node) {
        if (g->layer == this) return;
        g->detach();
    }
    Node *after = nullptr;
    for (Node *n = graphics.first; n && ((Graphic *)n->obj)->z < g->z; n = n->next) after = n;
    g->x -= screen->x;
    g->y -= screen->y;
    g->node = graphics.add(after, g);
    g->layer = this;
    screen->dirty();
}
void Layer::remove(Graphic *g) { if (g->layer == this) g->detach(); }
void Layer::sort() {                     // 0x41e9e0 (0x41e9c0: by z)
    graphics.sort([](Object *a, Object *b) { return ((Graphic *)a)->z - ((Graphic *)b)->z; });
    screen->dirty();
}

// ---------------------------------------------------------------- Graphic
Graphic::Graphic(int x_, int y_, int z_, int w_, int h_, int f6c_, bool vis, uint32_t col, int f78_, int f80_, int rot,
                 int hm, uint32_t f8c_)  // 0x411b90
    : ox(x_), oy(y_), oz(z_), ow(w_), oh(h_), hitmode(hm), x(x_), y(y_), z(z_), w(w_), h(h_), f6c(f6c_), color(col),
      f78(f78_), f80(f80_), visible(vis), rotation(rot), f8c(f8c_) {}
Graphic::~Graphic() { if (!tb_exiting()) detach(); }   // 0x402830
void Graphic::detach() {                 // 0x412200
    if (tb_exiting()) return;
    if (layer) { x += layer->screen->x; y += layer->screen->y; }
    dirty();
    layer = nullptr;
    if (node) Gorilla::tb_unlink_and_delete(node);
}
void Graphic::dirty() { if (layer) layer->screen->dirty(); }   // +0x34 (0x412010)
void Graphic::unload() {}
// Graphic::setHit (0x411e90), Graphic::hit (0x411f90) and the mask (0x418910): game/src/vision/hitmask.cpp.
bool Graphic::inBox(int px, int py) {    // 0x411ee0: the bounding box (inclusive, as the exe compares)
    int l = left(), r = right(), t = top(), b = bottom();
    if (rotation == 90 || rotation == 270) {          // 0x5a / 0x10e: w and h swap around the centre
        int sx = layer ? layer->screen->x : 0, sy = layer ? layer->screen->y : 0;
        l = x - (h >> 1) + sx; r = h + l;
        t = y - (w >> 1) + sy; b = w + t;
    }
    return l <= px && px <= r && t <= py && py <= b;
}
int Graphic::left() { return x - (w >> 1) + (layer ? layer->screen->x : 0); }    // 0x411d90
int Graphic::top() { return y - (h >> 1) + (layer ? layer->screen->y : 0); }     // 0x411dc0
int Graphic::right() { return left() + w; }                                      // 0x411df0
int Graphic::bottom() { return top() + h; }                                      // 0x411e30

// ---------------------------------------------------------------- Image
Image::Image(const char *n, int x_, int y_, int z_, int w_, int h_, int f6c_, bool vis)   // 0x4179e0
    : Graphic(x_, y_, z_, w_, h_, f6c_, vis, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {
    setName(n);
}
Image::~Image() { if (!tb_exiting()) unload(); }   // 0x417bb0

void Image::load() {                     // 0x416f80
    unload();
    pixelmap = (Pixelmap *)ras_find(name(), "sp5");
    if (!pixelmap) { vision_abort("No CD in drive. (%s.sp5)", name()); return; }   // the message comes from the text table
    x = pixelmap->x; y = pixelmap->y; z = pixelmap->z; w = pixelmap->w; h = pixelmap->h;
    ox = x; oy = y; ow = w; oh = h;
    if (!pixelmap->palette_name.empty()) {
        palette = (Palette *)ras_find(pixelmap->palette_name.c_str(), "plt");
        if (!palette) { vision_abort("Cannot open palette %s", pixelmap->palette_name.c_str()); return; }
    }
    if (!pixelmap->alpha_name.empty()) {
        alpha = (Alpha *)ras_find(pixelmap->alpha_name.c_str(), "alf");
        if (!alpha) { vision_abort("Cannot open alpha %s", pixelmap->alpha_name.c_str()); return; }
        alpha->owner = pixelmap;
        // pixelmap bpp 0x20: 0x416a40 premultiplies; no 32-bit sprites in this game's data
    }
}
void Image::unload() {                   // 0x417940
    if (tb_exiting()) return;
    if (pixelmap) pixelmap->release();
    if (palette) palette->release();
    if (alpha) alpha->release();
    pixelmap = nullptr; palette = nullptr; alpha = nullptr;
}
void Image::draw() {                     // 0x4179b0 -> 0x40d620
    if (!pixelmap) return;
    BlitParams p;
    p.x = x; p.y = y; p.z = z; p.w = w; p.h = h; p.rotation = rotation;
    p.translucency = translucency;
    p.pm = pixelmap; p.pal = palette; p.alpha = alpha;
    Canvas *c = current_canvas();        // 0x41eaa0
    if (!c->pixels) return;
    blit(p, c->pixels, c->w, c->h, c->pitch);
}

// ---------------------------------------------------------------- the blitter (0x40c8f0)
// port: the exe has ~30 specialised inner loops (0x405700..0x40c530: 8/16 bpp x raw/RLE/RLE16/byte-stream x
// opaque/alpha/translucent x scaled). They are written here as one row decoder (the record's own encoding, as
// formats.md and 0x417850/0x417920 describe) feeding one span writer that does the same per-pixel arithmetic:
// opaque copy, palette lookup for 8 bpp, alpha blend (a == 0xff copies), translucency blend, and the 16.16
// nearest-neighbour scaling of the scaled loops. Rotation (90/180/270) rotates the decoded rows (lane C).
namespace {
const uint32_t RM = 0xf800, GM = 0x7e0, BM = 0x1f;   // 0x5a67f4 / 0x5a67ec / 0x5a67e8

inline uint16_t mix(uint32_t dst, uint32_t src, uint32_t a) {    // the blend of 0x409fd0 etc.: a/256 of src
    uint32_t ia = 0x100 - a;
    return (uint16_t)(((((dst & BM) * ia + (src & BM) * a) >> 8) & BM) | ((((dst & GM) * ia + (src & GM) * a) >> 8) & GM) |
                      ((((dst & RM) * ia + (src & RM) * a) >> 8) & RM));
}

// Decodes one source image into rows: per pixel a 565 colour and an opacity (0 = transparent, 255 = opaque).
struct Decoded { int w, h; std::vector<uint16_t> c; std::vector<uint8_t> a; };

bool decode(const Pixelmap *pm, const Palette *pal, Decoded &d, bool as_alpha) {
    int w = pm->w, h = pm->h, bpp = pm->bpp, kind = pm->kind & ~0x30;
    const uint8_t *base = pm->data.data(), *end = base + pm->data.size();
    const uint8_t *p = base + pm->pixels;
    d.w = w; d.h = h;
    d.c.assign((size_t)w * h, 0);
    d.a.assign((size_t)w * h, 0);
    const uint16_t *lut = nullptr;
    if (bpp == 8 && pal && !as_alpha) lut = (const uint16_t *)(pal->data.data() + pal->entries);
    int bytes = bpp == 16 ? 2 : 1;
    auto px = [&](size_t i, const uint8_t *s) {
        if (s + bytes > end) return;
        if (as_alpha) { d.c[i] = s[0]; d.a[i] = 0xff; return; }
        if (bytes == 2) d.c[i] = (uint16_t)(s[0] | s[1] << 8);
        else d.c[i] = lut ? lut[s[0]] : (uint16_t)(s[0] * 0x0841u >> 3);
        d.a[i] = 0xff;
    };
    if (kind == 0) {
        for (int i = 0; i < w * h; i++, p += bytes) px(i, p);
        return true;
    }
    if (kind == 1 || kind == 2) {
        for (int r = 0; r < h; r++) {
            int col = 0;
            bool opaque = false;
            while (col < w && p < end) {
                uint32_t n = *p++;
                if (kind == 2 && n == 0xff && p + 2 <= end) { n = p[0] | p[1] << 8; p += 2; }
                if (opaque) {
                    for (uint32_t k = 0; k < n && col < w; k++, col++, p += bytes) px((size_t)r * w + col, p);
                } else col += n;
                opaque = !opaque;
            }
        }
        return true;
    }
    if (kind == 3) {                     // 0x417920: escape byte E; "E v n" = n x v; pixels equal to E are transparent
        uint8_t esc = *p++;
        size_t i = 0, n = (size_t)w * h;
        while (i < n && p < end) {
            uint8_t b = *p++;
            if (b == esc && p + 2 <= end) {
                uint8_t v = p[0], cnt = p[1];
                p += 2;
                // port: px() bounds-checks its source against the record's end, so a run value is passed as a one-byte
                // source inside the record (p - 2), not a local
                for (int k = 0; k < cnt && i < n; k++, i++) if (v != esc) px(i, p - 2);
            } else { px(i, &b); i++; }
        }
        return true;
    }
    vision_log("Unrecognized compression %d in %s", kind, pm->name());
    return false;
}

std::map<const Pixelmap *, Decoded> g_cache;   // port: decoded once per resource (released with it)
}  // namespace

void blit_forget(const Pixelmap *pm) { g_cache.erase(pm); }

void blit(const BlitParams &p, uint16_t *dst, int dw, int dh, int pitch) {
    const Pixelmap *pm = p.pm;
    if (!pm || p.w == 0 || p.h == 0 || pm->w == 0 || pm->h == 0) return;
    std::map<const Pixelmap *, Decoded>::iterator it = g_cache.find(pm);
    if (it == g_cache.end()) {
        Decoded d;
        if (!decode(pm, p.pal, d, false)) return;
        if (p.alpha && (pm->kind & 0x10)) {
            Decoded a;
            if (decode(p.alpha, nullptr, a, true) && a.w == d.w && a.h == d.h)
                for (size_t i = 0; i < d.a.size(); i++) if (d.a[i]) d.a[i] = (uint8_t)a.c[i];
        }
        it = g_cache.insert(std::make_pair(pm, d)).first;
    }
    const Decoded *dp = &it->second;
    Decoded rd;
    int ow = p.w, oh = p.h;                  // drawn size; != the pixelmap's size = the scaled loops
    if (p.rotation != 0) {
        // 0x40cb99: 90 (0x5a), 180 (0xb4) and 270 (0x10e) degrees clockwise; any other angle draws nothing.
        int rot = p.rotation;
        if (rot != 90 && rot != 180 && rot != 270) return;
        // 0x40ccb1: a sprite with the alpha flag (0x10) in the raw / RLE encodings is not drawn rotated.
        int enc = pm->kind & ~0x30;
        if ((pm->kind & 0x10) && enc < 3 && (pm->bpp == 8 || pm->bpp == 16)) return;
        // The exe decodes the sprite into a temporary buffer through the rotating destination strides
        // (0x566548 / 0x566554, set at 0x40cc26..0x40cca4) and then draws that as a raw 16-bit image.
        // port: rotate the decoded rows; the transparent pixels of RLE / escape sprites stay transparent
        // (the exe's temporary buffer keeps whatever it held there).
        const Decoded &src = *dp;
        int W = src.w, H = src.h;
        bool swap = rot != 180;
        rd.w = swap ? H : W; rd.h = swap ? W : H;
        rd.c.assign((size_t)W * H, 0); rd.a.assign((size_t)W * H, 0);
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                int dx, dy;                                  // where source (x, y) lands
                if (rot == 90) { dx = H - 1 - y; dy = x; }       // row 0 -> the rightmost column, top to bottom
                else if (rot == 180) { dx = W - 1 - x; dy = H - 1 - y; }
                else { dx = y; dy = W - 1 - x; }                 // 270: row 0 -> the leftmost column, bottom to top
                rd.c[(size_t)dy * rd.w + dx] = src.c[(size_t)y * W + x];
                rd.a[(size_t)dy * rd.w + dx] = src.a[(size_t)y * W + x];
            }
        dp = &rd;
        if (swap) { ow = p.h; oh = p.w; }                    // 0x40c9ff: the swapped extents
    }
    const Decoded &d = *dp;
    int pmw = d.w, pmh = d.h;
    if (pmw == 0 || pmh == 0) return;
    int x0 = (dw - ow) / 2 + p.x, y0 = (dh - oh) / 2 + p.y;   // centred on the canvas (0x56657c / 0x566580)
    int t = p.translucency;
    // 16.16 steps of the scaled loops (0x566548 / 0x566554)
    uint32_t sx = ow == 1 || pmw == 1 ? 0 : (uint32_t)(((ow - 1) / 2 - 0x10000 + pmw * 0x10000) / (ow - 1));
    uint32_t sy = oh == 1 || pmh == 1 ? 0 : (uint32_t)(((oh - 1) / 2 - 0x10000 + pmh * 0x10000) / (oh - 1));
    bool scaled = ow != pmw || oh != pmh;
    for (int r = 0; r < oh; r++) {
        int dy = y0 + r;
        if (dy < 0 || dy >= dh) continue;
        int srow = scaled ? (int)((0x8000u + sy * r) >> 16) : r;
        if (srow >= d.h) srow = d.h - 1;
        uint16_t *out = dst + (size_t)dy * pitch;
        const uint16_t *sc = &d.c[(size_t)srow * d.w];
        const uint8_t *sa = &d.a[(size_t)srow * d.w];
        int c0 = x0 < 0 ? -x0 : 0, c1 = x0 + ow > dw ? dw - x0 : ow;
        for (int c = c0; c < c1; c++) {
            int scol = scaled ? (int)((0x8000u + sx * c) >> 16) : c;
            if (scol >= d.w) scol = d.w - 1;
            uint32_t a = sa[scol];
            if (!a) continue;
            uint16_t s = sc[scol];
            uint16_t &o = out[x0 + c];
            if (t != 0 && t != 0xff) a = a == 0xff ? (uint32_t)t : a * t >> 8;   // port: alpha x translucency
            o = a == 0xff ? s : mix(o, s, a);
        }
    }
}

// ---------------------------------------------------------------- the Screen service (vtable 0x513a1c)
struct ScreenKind : ServiceKind {
    ScreenKind() : ServiceKind("Screen", -10000) {}
    void init() override { screen_init(); }          // 0x41efa0 -> 0x40dc90
    void shutdown() override { screen_shutdown(); }  // 0x41efb0 -> 0x40e2e0
    void frame() override {                          // 0x41efc0
        if (g_movie_playing) return;
        if (!g_back) return;
        if (g_bg) g_back->Blt(0, 0, g_bg);           // 0x40e360
        lock_canvas();                               // 0x40e390
        for (Node *n = screen_list().first; n; n = n->next) ((Screen *)n->obj)->draw();   // +0x28
        // the cursor (0x5a69c4, drawn last when visible): the game's cursor class sets it (port: TODO with the cursor)
        unlock_canvas();                             // 0x40e620
        // 0x5a675c / 0x5a666c: a GDI pass over the text graphics (lane C's DC) when any are shown
        g_primary->Blt(0, 0, g_back);                // the flip (vtbl +0x1c): presents
    }
};

void register_screen_kind() { static ScreenKind k; }

}  // namespace Vision
