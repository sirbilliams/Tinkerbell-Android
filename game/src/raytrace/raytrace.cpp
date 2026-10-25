// The 3D engine, reconstructed from "Tinker Bell.exe" (see raytrace.h and docs/raytrace-notes.md).
// The x87 code is followed from the disassembly where the decompiler lost operands; float vs double precision is
// kept as the exe stores it (float locals, double fields).
#include "raytrace.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tb.h"
#include <SDL.h>
#include "../../../platform/dsound.h"   // tb_fopen_ci

namespace Raytrace {

static const float kTiny = 1e-8f;           // 0x322bcc77: replaces a zero cos/sin in the angle table, for good
static const double kTinyDist = 1e-6;       // 0x3eb0c6f7a0b5ed8d: replaces a zero distance
static const float kProj = 590.0f;          // 0x513d24: the projection constant (screen distance)

uint8_t seg_init(Seg *s, float x1, float z1, float x2, float z2) {   // 0x427560
    uint8_t f = 0;
    if (x2 <= x1) { f = 2; s->minx = x2; } else s->minx = x1;
    if (z2 <= z1) { f += 1; s->minz = z2; } else s->minz = z1;
    s->maxx = x1 <= x2 ? x2 : x1;
    s->maxz = z1 <= z2 ? z2 : z1;
    if (x1 == x2) {
        s->vertical = true;
        s->icpt = x1;
        return f;
    }
    s->vertical = false;
    s->slope = (z2 - z1) / (x2 - x1);
    s->icpt = z1 - s->slope * x1;
    return f;
}

RayTrace::RayTrace() : Vision::Screen(0, 0, 0, 0, 0, true, true) {}
RayTrace::~RayTrace() { release(); }

// 0x429980 (from the scene's leave): everything the loaders made.
// port: frees in one place what the exe frees across 0x429980 and its helpers (same set of objects).
void RayTrace::release() {
    for (WallObject *o : walls) delete o;
    walls.clear();
    for (Seg *s : segs) delete s;
    segs.clear();
    listA = listB = nullptr;
    for (Vision::Image *t : tiles) delete t;
    tiles.clear();
    grid.clear();
    for (Vision::Pixelmap *t : textures) if (t) t->release();   // the wall textures found by 0x426f00
    textures.clear();
    mirrored.clear();
    for (Sprite3D *s : sprites) delete s;
    sprites.clear();
    for (Box *b : boxes) delete b;
    boxes.clear();
    for (Seg *s : bsegs) delete s;
    bsegs.clear();
    boundary = nullptr;
    for (Barrier *b : barriers) delete b;
    barriers.clear();
    ready = false;
}

// 0x4279a0: the corner `a` shares with `b` (a's two ends by its direction flags; the one that is also an end of b,
// else a's other end).
void RayTrace::fallback(Seg *a, Seg *b, float *x, float *z) {
    float p1x = a->minx, p2x = a->maxx, p1z, p2z;
    if (a->flags == 0 || a->flags == 3) { p1z = a->minz; p2z = a->maxz; }
    else { p1z = a->maxz; p2z = a->minz; }
    bool shared;
    if (b->flags == 0 || b->flags == 3)
        shared = (p1x == b->minx && b->minz == p1z) || (p1x == b->maxx && b->maxz == p1z);
    else
        shared = (p1x == b->minx && b->maxz == p1z) || (p1x == b->maxx && b->minz == p1z);
    *x = shared ? p1x : p2x;
    *z = shared ? p1z : p2z;
}

static float &guard(float &v) {             // a zero table entry becomes kTiny (the exe writes it back)
    if (v == 0.0f) v = kTiny;
    return v;
}

Seg *RayTrace::cast(int col, const Seg &ray, Seg *start, Column *out, Seg *end) {   // 0x425f60
    if (!end) end = start->prev;
    double best = INFINITY;
    Seg *hit = nullptr;
    float hx = 0, hz = 0;
    for (Seg *s = start;; s = s->next) {
        if (ray.minx <= s->maxx && s->minx <= ray.maxx && ray.minz <= s->maxz && s->minz <= ray.maxz) {
            bool cand = false;
            float x = 0, z = 0;
            if (!ray.vertical) {
                if (!s->vertical) {
                    float d = ray.slope - s->slope;
                    if (d == 0.0f) {
                        if (ray.icpt == s->icpt) {          // collinear: the middle of the overlap
                            bool rmax = ray.maxx < s->maxx;
                            if (s->minx < ray.minx) {
                                float ex, ez;
                                if (rmax) { ex = ray.maxx; ez = ray.maxz; } else { ex = s->maxx; ez = s->maxz; }
                                x = (ray.minx + ex) * 0.5f;
                                z = ez + ray.minz;
                            } else if (rmax) {
                                x = (s->minx + ray.maxx) * 0.5f;
                                z = ray.maxz + s->minz;
                            } else {
                                x = (s->minx + s->maxx) * 0.5f;
                                z = s->maxz + s->minz;
                            }
                            z = z * 0.5f;
                            cand = true;
                        }
                    } else {
                        x = (s->icpt - ray.icpt) / d;
                        if (ray.minx <= x && x <= ray.maxx && s->minx <= x && x <= s->maxx) {
                            z = x * ray.slope + ray.icpt;
                            cand = true;
                        }
                    }
                } else {
                    z = ray.slope * s->icpt + ray.icpt;
                    if (s->minz <= z && z <= s->maxz) { x = s->icpt; cand = true; }
                }
            } else if (!s->vertical) {
                z = s->slope * ray.icpt + s->icpt;
                if (ray.minz <= z && z <= ray.maxz) { x = ray.icpt; cand = true; }
            } else if (s->icpt == ray.icpt) {
                z = ray.minz;
                x = ray.icpt;
                cand = true;
            }
            if (cand) {
                double dx = (double)x - camx, dz = (double)z - camz;
                double d2 = dx * dx + dz * dz;
                if (d2 < best) { best = d2; hit = s; hx = x; hz = z; }
            }
        }
        if (s == end) break;
    }
    out->col = col;
    while (!hit) {
        Seg *s = start;
        for (;;) {
            if (!s->tex) {                          // an invisible wall: the column shows nothing
                out->tw = s->tw;
                out->base = s->base;
                out->dist = 1.7976931348623157e308;
                out->texcol = nullptr;
                out->top = 0x7fffffff;
                out->topofs = (int)((uint32_t)w * 0x7fffffffu);
                return s;
            }
            if (s == end) break;
            s = s->next;
        }
        // port: the exe writes this corner into temporaries it never reads and goes on with the hit point unset.
        fallback(start, end, &hx, &hz);
        hit = start;
    }
    out->tw = hit->tw;
    out->base = hit->base;
    int a = view - col;
    if (a < 0) a += (int)angles.size();
    // x87 at the exe's 53-bit precision: hx/hz were stored as floats, the differences stay doubles
    double dx = (double)hx - camx;
    double dz = (double)hz - camz;
    if (fabs(dz) < fabs(dx)) out->dist = dx / guard(angles[a].dx);
    else out->dist = dz / guard(angles[a].dz);
    // the texture column: how far along the wall, by its longer extent, from the end its flags say
    double u;
    double spanx = fabs((double)hit->maxx - hit->minx);
    double spanz = fabs((double)hit->maxz - hit->minz);
    if (spanx > spanz) u = ((hit->flags & 2) ? ((double)hit->maxx - hx) : ((double)hx - hit->minx)) / spanx;
    else u = ((hit->flags & 1) ? ((double)hit->maxz - hz) : ((double)hz - hit->minz)) / spanz;
    u *= (double)(int64_t)(uint32_t)hit->th;
    if (!hit->tex) {
        out->top = 0x7fffffff;
        out->texcol = nullptr;
        out->topofs = (int)((uint32_t)w * (uint32_t)out->top);
        return hit;
    }
    if (maxdist < out->dist) maxdist = out->dist;
    out->texcol = hit->tex + (int)u * hit->tw;    // 0x4f3948 (ftol, truncation)
    if (out->dist == 0.0) out->dist = kTinyDist;
    double sc = kProj / out->dist;
    out->scale = sc;
    int a2 = col - w / 2;
    if (a2 < 0) a2 += (int)angles.size();
    if (angles[a2].dx == 0.0f) out->scale = 0;
    else out->scale = sc / angles[a2].dx;
    out->top = horizon - (int)((double)(eye - out->base) * out->scale);
    out->topofs = w * out->top;
    return hit;
}

// 0x427130: 6 * width steps per turn (a 60 degree view, pi/3 = 0x513d28 * 0x513d34), the four axes exact, and
// 4096-fixed-point copies (0x513d30).
void RayTrace::initAngles() {
    const float pi = 3.14159274f;                   // 0x513d28
    int n = (int)((double)w * pi * 2.0 / ((double)pi * 0.333333343f));
    angles.assign(n, Angle());
    float step = (float)((double)pi * 2.0 / (double)n);
    double ang = 0.0;
    for (int i = 0; i < n; i++) {
        angles[i].a = (float)ang;
        angles[i].dx = (float)cos(ang);
        angles[i].dz = (float)sin(ang);
        ang += step;
    }
    angles[0].dx = 1.0f; angles[0].dz = 0.0f;
    angles[(unsigned)n >> 1].dx = -1.0f; angles[(unsigned)n >> 1].dz = 0.0f;
    angles[(unsigned)n >> 2].dx = 0.0f; angles[(unsigned)n >> 2].dz = 1.0f;
    angles[(unsigned)(n * 3) >> 2].dx = 0.0f; angles[(unsigned)(n * 3) >> 2].dz = -1.0f;
    for (int i = 0; i < n; i++) {
        angles[i].idx = (int)((double)angles[i].dx * 4096.0f);
        angles[i].idz = (int)((double)angles[i].dz * 4096.0f);
    }
}

void RayTrace::subdivide(unsigned lo, unsigned hi, Seg *startA, Seg *startB, Seg *endA, Seg *endB) {   // 0x426480
    unsigned mid = (lo + hi) >> 1;
    int a = view - (int)mid;
    if (a < 0) a += (int)angles.size();
    Seg ray;
    float farz = (float)(angles[a].dz * range + camz);
    float farx = (float)(angles[a].dx * range + camx);
    seg_init(&ray, camx, camz, farx, farz);
    Seg *hitA = cast((int)mid, ray, startA, nextColumn(), endA);
    Seg *hitB = cast((int)mid, ray, startB, nextColumn(), endB);
    if (mid - lo > 1) subdivide(lo, mid, startA, startB, hitA, hitB);
    if (hi - mid > 1) subdivide(mid, hi, hitA, hitB, endA, endB);
}

// ---------------------------------------------------------------- loading
static bool rd32(FILE *f, void *p) { return fread(p, 4, 1, f) == 1; }      // 0x4101e0 (File::read, 4 bytes)
static uint16_t *tex_pixels(Vision::Pixelmap *pm) {                        // record + 4 + u32(record + 0x80)
    return pm ? (uint16_t *)(pm->data.data() + pm->pixels) : nullptr;
}

// 0x426f00: the texture names; a name with an extension is looked up without it; a missing "...r" is the base
// texture mirrored.
static Vision::Pixelmap *find_tex(const char *n40, bool *mirror) {
    std::string n(n40);
    size_t dot = n.find('.');
    if (dot != std::string::npos) n = n.substr(0, dot);
    *mirror = false;
    Vision::Pixelmap *pm = (Vision::Pixelmap *)Vision::ras_find(n.c_str(), "sp5");
    if (!pm) {
        Vision::vision_log("Could not find texture %s", n.c_str());
        if (!n.empty() && (n.back() == 'r' || n.back() == 'R')) {
            n.pop_back();
            pm = (Vision::Pixelmap *)Vision::ras_find(n.c_str(), "sp5");
            if (pm) *mirror = true;
        }
    }
    return pm;
}

// 0x428580: the walls, each textured one with its wall object (0x42e5d0).
bool RayTrace::loadWalls(const char *name) {
    char path[300];
    snprintf(path, sizeof path, "data\\%s.rtc", name);                  // 0x535a60
    FILE *f = tb_fopen_ci(path);
    if (!f) { Vision::vision_log("RayTrace: cannot open %s", path); return false; }
    fseek(f, 0x58, SEEK_SET);                                          // past the record header
    uint32_t n = 0;
    rd32(f, &n);                                                       // +0xb0
    textures.assign(n, nullptr);
    mirrored.assign(n, 0);
    for (uint32_t i = 0; i < n; i++) {
        char nm[41] = {0};
        if (fread(nm, 40, 1, f) != 1) break;                           // 0x410180(buf, 0x28)
        bool m = false;
        textures[i] = find_tex(nm, &m);
        mirrored[i] = m;
    }
    rd32(f, &ground_tex);
    rd32(f, &sky_tex);
    for (int list = 0; list < 2; list++) {
        Seg *first = nullptr, *last = nullptr;
        for (;;) {
            int32_t t;
            float x1, z1, x2, z2;
            if (!rd32(f, &t) || t == -1) break;
            Seg *sg = new Seg();
            segs.push_back(sg);
            rd32(f, &x1); rd32(f, &z1); rd32(f, &x2); rd32(f, &z2);
            rd32(f, &sg->base);
            if (!first) first = sg;
            else { last->next = sg; sg->prev = last; }
            last = sg;
            sg->flags = seg_init(sg, x1, z1, x2, z2);
            if (t == -2) { sg->tex = nullptr; }
            else if (t >= 0 && t < (int)textures.size() && textures[t]) {
                if (mirrored[t]) sg->flags ^= 3;
                Vision::Pixelmap *pm = textures[t];
                sg->tw = pm->w;                                        // +0x64
                sg->th = pm->h;                                        // +0x68
                sg->tex = tex_pixels(pm);
                make_wall_object(this, pm, x1, z1, x2, z2);
            }
            sg->angle = atan2((double)z2 - z1, (double)x2 - x1) - (double)3.14159274f * 0.5f;   // 0x513d28 * 0x513298
        }
        if (first) { last->next = first; first->prev = last; }         // circular
        if (list == 0) listA = first; else listB = first;
    }
    range = wallRange();                                               // +0x540
    fclose(f);
    return true;
}

// 0x42cc80: x and z offsets, the tile images, then 128 x 128 bytes naming each cell's tile.
bool RayTrace::loadGround(const char *rel) {
    FILE *f = tb_fopen_ci(rel);
    if (!f) { Vision::vision_log("RayTrace: cannot open %s", rel); return false; }
    rd32(f, &ground_x);                                                // +0x4f8
    rd32(f, &ground_z);                                                // +0x4fc
    int32_t n = 0;
    rd32(f, &n);                                                       // +0xb8
    std::vector<const uint16_t *> tilepix;
    for (int i = 0; i < n; i++) {
        char nm[41] = {0};
        if (fread(nm, 40, 1, f) != 1) break;
        char *dot = strchr(nm, '.');
        if (dot) *dot = 0;                                             // the name without ".sp5"
        Vision::Image *im = new Vision::Image(nm, 0, 0, 0, 0, 0, 0, true);
        im->load();
        tiles.push_back(im);
        Vision::Pixelmap *pm = im->pixelmap;
        if (i == 0 && pm) {                                            // the masks from the first tile's width
            int b = 0;
            g_umask = 0;
            for (uint32_t w2 = (uint32_t)pm->w; (w2 >>= 1) != 0;) { b++; g_umask = g_umask << 1 | 1; }
            g_tshiftx = b + 16;
            g_vshift = 16 - b;
            g_vmask = g_umask << b;
            int c = 0;
            uint32_t m = 0;
            for (uint32_t k = 0x40;; ) { c++; k >>= 1; m = m << 1 | 1; if (!k) break; }
            g_tmaskx = m;
            g_tshifty = g_tshiftx - c;
            g_tmasky = m << c;
        }
        tilepix.push_back(pm ? tex_pixels(pm) : nullptr);
    }
    grid.assign(0x4000, nullptr);
    for (int i = 0; i < 0x4000; i++) {
        uint8_t b = 0;
        if (fread(&b, 1, 1, f) != 1) break;                            // 0x4101b0
        grid[i] = b < tilepix.size() ? tilepix[b] : nullptr;
    }
    fclose(f);
    return true;
}

// 0x427620: per half-screen column (from the edge in), the ground distance of each row below the horizon.
void RayTrace::groundTable() {
    if (ground_mode == 0 || (float)eye_built == eyef) return;
    eye_built = (int)eyef;
    int half_w = w / 2;
    unsigned half_h = (unsigned)(h / 2);
    if (rowscale.empty()) {
        rowscale.resize(half_h);
        for (int r = (int)half_h; r > 0; r--) rowscale[r - 1] = (int)((half_h << 8) / (unsigned)r);
    }
    gtable.resize((size_t)half_w * half_h);
    int *p = gtable.data();
    // (160 + eye) * 590 * 16 (0x513d1c, 0x513d24, 0x513d3c): the distance scale (focal length 590, 1/16 units)
    double eye = (((double)160.0f + eyef) * 590.0f) * 16.0f;
    for (int c = half_w, k = half_w * 60; c != 0; c--, k -= 60) {
        double ang = (double)k * 3.14159274f * 8.68055577e-06f;        // 0x513d28 * 0x513d38: k / 1920 * pi
        int d = (int)(eye / cos(ang));
        for (int r = (int)half_h - 1; r >= 0; r--)
            *p++ = (int)((uint32_t)rowscale[r] * (((uint32_t)d << 7) / half_h) >> 15);
    }
}

// ---------------------------------------------------------------- drawing
// The canvas the renderer draws into (0x5a95ac = 0x41ead0(): the current canvas' pixels).
static uint16_t *canvas_pixels() {
    Vision::Canvas *c = Vision::current_canvas();
    return c ? c->pixels : nullptr;
}

// 0x42a140: the sky picture across the top, scrolled with the view (one turn = the picture's width), wrapping.
void RayTrace::drawSky() {
    sky_rows = 480 - g_sky_top;
    uint16_t *dst0 = canvas_pixels();
    if (!dst0 || sky_tex < 0 || sky_tex >= (int)textures.size() || !textures[sky_tex]) return;
    // port: TODO +0x525 (the panorama variant 0x42a340)
    Vision::Pixelmap *pm = textures[sky_tex];
    uint32_t tw = (uint32_t)pm->w, th = (uint32_t)pm->h;
    const uint16_t *src = tex_pixels(pm);
    int cut = (sky_clip && (uint32_t)sky_rows < th) ? (int)(th - (uint32_t)sky_rows) : 0;
    uint32_t n = (uint32_t)angles.size();
    uint32_t a = (uint32_t)(sky_angle - view) - 1 + n;
    if (n <= a) a -= n;
    uint32_t x0 = (uint32_t)(((uint64_t)a * tw) / n) & 0xffffffffu;
    uint32_t rows = th - (uint32_t)cut;
    if (tw < (uint32_t)w + x0) {
        uint32_t first = tw - x0;                                       // to the picture's right edge
        for (uint32_t r = 0; r < rows; r++)
            memcpy(dst0 + r * w, src + r * tw + x0, first * 2);
        for (uint32_t r = 0; r < rows; r++)                             // then from its left edge
            memcpy(dst0 + r * w + first, src + r * tw, ((uint32_t)w - first) * 2);
    } else {
        for (uint32_t r = 0; r < rows; r++)
            memcpy(dst0 + r * w, src + r * tw + x0, (size_t)w * 2);
    }
}

// 0x42bf50: for each cast column, from the bottom row up to the horizon, the ground cell under that row's distance.
void RayTrace::drawGround2() {
    uint16_t *pix = canvas_pixels();
    if (!pix || grid.empty() || gtable.empty()) return;
    int last = w - 1, half = w / 2;
    int u0 = (int)(((double)(int64_t)ground_x + camx) * 65536.0f);     // 0x513520
    int v0 = (int)(((double)(int64_t)ground_z + camz) * 65536.0f);
    uint16_t *bottom = pix + (size_t)(h - 1) * w;                      // +0xa0
    for (int i = 0; i < ncolumns; i += 2) {
        int col = columns[i].col;
        uint16_t *p = bottom + col;
        int a = view - col;
        if (a < 0) a += (int)angles.size();
        int m = col >= half ? last - col : col;
        const int *t = gtable.data() + (size_t)horizon * m;
        uint16_t *stop = p - (size_t)w * horizon;
        if (ground_rows < horizon) ground_rows = horizon;
        int dx = angles[a].idx, dz = angles[a].idz;
        while (stop < p) {
            int d = *t++;
            uint32_t u = (uint32_t)(d * dx + u0), v = (uint32_t)(d * dz + v0);
            const uint16_t *tile = grid[((u >> g_tshiftx) & g_tmaskx) | ((v >> g_tshifty) & g_tmasky)];
            *p = tile ? tile[((v >> g_vshift) & g_vmask) | ((u >> 16) & g_umask)] : 0;
            p -= w;
        }
    }
}

// The column part of 0x426710: rays at the two screen edges for both wall lists, then the columns between.
void RayTrace::castColumns() {
    if (!listA) return;
    if ((int)columns.size() < w * 2) columns.resize((size_t)w * 2);
    ncolumns = 0;
    Seg ray;
    float farz = (float)(angles[view].dz * range + camz);
    float farx = (float)(angles[view].dx * range + camx);
    seg_init(&ray, camx, camz, farx, farz);
    Seg *a0 = cast(0, ray, listA, nextColumn(), nullptr);
    Seg *b0 = listB ? cast(0, ray, listB, nextColumn(), nullptr) : nullptr;
    int a = view - w + 1;
    if (a < 0) a += (int)angles.size();
    farz = (float)(angles[a].dz * range + camz);
    farx = (float)(angles[a].dx * range + camx);
    seg_init(&ray, camx, camz, farx, farz);
    Seg *a1 = cast(w - 1, ray, listA, nextColumn(), nullptr);
    Seg *b1 = listB ? cast(w - 1, ray, listB, nextColumn(), nullptr) : nullptr;
    if (!listB) return;                                               // port: London has both lists
    subdivide(0, (unsigned)(w - 1), a0, b0, a1, b1);
}

// 0x4281a0: everything for the scene `name` (the walls pack is current).
bool RayTrace::load(const char *name) {
    ready = false;                                                     // +0xa4
    listA = listB = nullptr;
    last_time = Vision::frame_time();
    eyef = 0;
    sky_angle = 0;
    sky_clip = true;                                                   // +0x524
    ground_x = ground_z = 0;
    initAngles();
    walls.clear();
    if (!loadWalls(name)) return false;
    columns.assign((size_t)w * 2, Column());                           // +0x548: w * 0x60 bytes
    initCollision();
    for (Sprite3D *sp : sprites) delete sp;                            // the sprite vector is emptied
    sprites.clear();
    if (speedRating() < 4) { if (ground_tex || sky_tex) ground_mode = 3; }   // port: mode 3 isn't ported
    else ground_mode = 2;
    groundTable();
    return true;
}

// 0x426710: the frame: the columns again if the camera moved, sky, backdrop, ground, then walls and sprites.
void RayTrace::draw() {
    if (!visible || !ready) return;
    frames++;
    uint32_t now = Vision::frame_time();
    frame_dt = now - last_time;
    if (frame_dt == 0) frame_dt = 1;
    last_time = now;
    horizon = h / 2;
    // port: the sprite box refresh (+0xa0 sprites, 0x4294b0) is not in yet; London's sprites don't set it
    if (moved) {
        maxdist = 1.0;                                                 // +0x550
        moved = false;
        ncolumns = 0;
        eye = (int)eyef;
        castColumns();
    }
    int e8 = (int)eyef;
    g_sky_top = 240 - e8 / 8;                                          // (the exe's +0x51c sum here is
    if (g_sky_top < 0) g_sky_top = 0;                                  //  overwritten by the sky)
    drawSky();
    if (backdrop) {
        uint32_t n = (uint32_t)angles.size();
        uint32_t bw = (uint32_t)backdrop->w;
        uint32_t a = (uint32_t)(sky_angle - view) - 1 + n;
        if (n <= a) a -= n;
        uint32_t off = (uint32_t)(((uint64_t)a * bw) / n);
        backdrop->x = (int)(bw - (uint32_t)w) / 2 - (int)off;
        backdrop->draw();
        if (bw < (uint32_t)w + off) { backdrop->x += (int)bw; backdrop->draw(); }
    }
    if (!no_ground) drawGround();                                      // 0x42c460
    if (overlay) overlay->draw();
    drawWalls();
}

// 0x42bc30
uint32_t RayTrace::idleTime() {
    if (still_since != 0) {
        uint32_t t = Vision::frame_time() - still_since;
        if (t > 60) return t;
    }
    return 0;
}

// 0x42c460: the ground; when the camera has stood still for 60 ms the drawn rows are kept and copied back
// instead of cast again. port: only mode 2 (0x42bf50) is ported (the mode for a processor rating >= 4).
void RayTrace::drawGround() {
    bool keep = false;
    uint16_t *pix = canvas_pixels();
    if (gc_view == view && gc_x == camx && gc_z == camz && gc_eye == eyef) {
        if (gc_valid) {
            if (pix && !gc_pixels.empty())
                memcpy(pix + (size_t)(h - ground_rows) * w, gc_pixels.data(), (size_t)w * ground_rows * 2);
            return;
        }
        if (still_since == 0) still_since = Vision::frame_time();
        else if (Vision::frame_time() - still_since > 60) keep = true;
    } else {
        still_since = 0;
    }
    gc_view = view;
    gc_x = camx;
    gc_valid = false;
    ground_rows = 0;
    gc_z = camz;
    gc_eye = eyef;
    drawGround2();
    if (keep) {
        move_start = 0;                                                // +0x8c
        if (ground_rows != 0 && pix) {
            gc_pixels.assign(pix + (size_t)(h - ground_rows) * w, pix + (size_t)h * w);
            gc_valid = true;
        }
    }
}

// 0x4190c0, run once (0x419170): copy 600 KB ten times; 10 - ms / 25, at least 1.
int RayTrace::speedRating() {
    static int rating = -1;
    if (rating == -1) {
        uint32_t t0 = SDL_GetTicks();
        std::vector<uint32_t> a(0x25800), b(0x25800);
        for (int k = 0; k < 10; k++) {
            memset(a.data(), 0, a.size() * 4);
            memcpy(b.data(), a.data(), a.size() * 4);
        }
        rating = 10 - (int)(SDL_GetTicks() - t0) / 0x19;
        if (rating < 1) rating = 1;
        Vision::vision_log("Processor Speed rating (1-10) is %d", rating);
    }
    return rating;
}

}  // namespace Raytrace
