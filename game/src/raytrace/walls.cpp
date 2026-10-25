// The 3D engine's textured walls, reconstructed from "Tinker Bell.exe": each textured wall of the .rtc is a
// WallObject drawn as projected quads (TrapezoidRenderer), one per slice of the 60 degree view, sorted far to near.
// x87 precision as in raytrace.cpp: values the exe keeps on the FPU stack are doubles, stores to float locals round.
#include "raytrace.h"
#include "trapezoid.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>

namespace Vision { void canvas_set_mode(int m); }

namespace Raytrace {

static const float kPi = 3.14159274f;               // 0x513d28
static const float kDeg = 0.00555555569f;           // 0x513d90: 1/180
static const float kTwoPi = 6.28318548f;            // 0x5a95b8 (2 * 0x513d28, static init 0x42cff0)

// ---------------------------------------------------------------- Line (0x42f850..0x42f9f0)
static bool within(float v, float a, float b) {
    if (a < b) return !(v < a) && v <= b;
    return !(v < b) && v <= a;
}

bool Line::intercept(float *b) const {               // 0x42f850
    double dx = (double)x2 - x1;
    if (dx == 0.0) return false;
    *b = (float)((double)z1 - ((double)z2 - z1) / dx * x1);
    return true;
}
bool Line::inX(float x) const { return within(x, x1, x2); }   // 0x42f890
bool Line::inZ(float z) const { return within(z, z1, z2); }   // 0x42f8f0

bool Line::zAt(float x, float *z) const {            // 0x42f950
    if (!inX(x)) return false;
    double dx = (double)x2 - x1;
    if (dx == 0.0) return x == x1;                   // (z left as it was)
    *z = (float)(((double)z2 - z1) * ((double)x - x1) / dx + z1);
    return true;
}

// 0x42f9f0: this line against `o`; *x, *z = the meeting point.
// port: two branches the exe can't reach (a line found non-vertical, then tested again) are left out.
bool Line::intersect(const Line &o, float *x, float *z) const {
    double dx = (double)x2 - x1;
    if (dx != 0.0) {
        float s = (float)(((double)z2 - z1) / dx);
        double odx = (double)o.x2 - o.x1;
        if (odx == 0.0) {                            // the other is vertical
            *x = o.x1;
            float zz;
            if (!zAt(o.x1, &zz)) return false;
            if (!o.inZ(zz)) return false;
            *z = zz;
            return true;
        }
        float os = (float)(((double)o.z2 - o.z1) / odx);
        float b = 0, ob = 0;
        intercept(&b);
        o.intercept(&ob);
        double ds = (double)s - os;
        if (ds == 0.0) return false;                 // parallel
        float xi = (float)(((double)ob - b) / ds);
        if (!inX(xi) || !o.inX(xi)) return false;
        *x = xi;
        *z = (float)((double)xi * s + b);
        return true;
    }
    *x = x1;                                         // this one is vertical
    double odx = (double)o.x2 - o.x1;
    if (odx == 0.0) {                                // both: overlapping z ranges
        if (x1 != o.x1) return false;
        if (within(o.z1, z1, z2)) { *z = o.z1; return true; }
        if (within(o.z2, z1, z2)) { *z = o.z2; return true; }
        if (within(z1, o.z1, o.z2)) { *z = z1; return true; }
        if (within(z2, o.z1, o.z2)) { *z = z2; return true; }
        return false;
    }
    float xv = x1;
    if (!o.inX(xv)) return false;
    float zz = (float)(((double)o.z2 - o.z1) * ((double)xv - o.x1) / odx + o.z1);
    if (!within(zz, z1, z2)) return false;
    *z = zz;
    return true;
}

// ---------------------------------------------------------------- WallObject
WallObject::WallObject(const char *n, const char *p) : name(n), prefix(p) {   // 0x42e5d0
    anim = new Vision::Animation(prefix.c_str());   // 0x404c20
}
WallObject::~WallObject() { delete anim; }

// The wall objects for a textured wall (part of the loader 0x428580): "wall_<texture>_<count + 1>", found again by
// name (0x423840) and loaded once; its corners are the wall's end points truncated to whole units.
void make_wall_object(RayTrace *rt, Vision::Pixelmap *tex, float x1, float z1, float x2, float z2) {
    int iz2 = (int)z2, ix2 = (int)x2, iz1 = (int)z1, ix1 = (int)x1;
    char nm[300];
    snprintf(nm, sizeof nm, "wall_%s_%2.2d", tex->name(), (int)rt->walls.size() + 1);   // 0x535a50
    WallObject *o = nullptr;
    for (WallObject *w : rt->walls)
        if (strcasecmp(w->name.c_str(), nm) == 0) { o = w; break; }
    if (!o) {
        o = new WallObject(nm, tex->name());
        rt->walls.push_back(o);
    }
    if (!o->loaded) {
        o->anim->load();                             // v15
        o->anim->mode = 1;                           // +0x104: loop
        o->last = Vision::frame_time();
        o->loaded = true;
        o->on = true;
    }
    o->bottom = 0;
    o->x1 = (float)ix1;
    o->z1 = (float)iz1;
    o->x2 = (float)ix2;
    o->z2 = (float)iz2;
    Vision::Image *f = o->anim->frame(o->anim->current());
    o->top = (float)((double)(f ? f->ow : 0) * 2.47f);   // 0x513d40; port: guards a texture without frames
}

// ---------------------------------------------------------------- camera and projection
static int round_(double v) { return (int)floor(v + 0.5); }   // 0x425b50

void RayTrace::setCamera(float x, float e, float z, float deg) {   // 0x42a570
    moved = ready = true;
    int r = round_(x);
    if (fabs((double)x - r) < 0.01) x = (float)((double)r + 0.01f);   // never exactly on a whole unit
    r = round_(e);
    if (fabs((double)e - r) < 0.01) e = (float)((double)r + 0.01f);
    float rz = (float)round_(z);
    if (fabs((double)z - rz) < 0.01) z = (float)((double)rz + 0.01f);
    camz = z;
    camx = x;
    eyef = e;
    angf = (float)(((double)deg + 30.0f) * kPi * kDeg);
    while (angf < 0.0f) angf = (float)((double)kTwoPi + angf);
    while (!(angf < kTwoPi)) angf = (float)((double)angf - kTwoPi);
    int n = (int)angles.size();
    view = (int)((double)n * angf / kTwoPi);
    int c = view - w / 2;
    int a = c - 1;
    if (a < 0) {
        a += n;
        if (c < 0) c += n;
    }
    angf = (float)((c == 0 ? (double)angles[1].a : (double)angles[a].a + angles[c].a) * 0.5f);
    groundTable();
}

// 0x42df80: the screen column (relative to the centre) and row of the point (x, z) at height h (times k percent).
void RayTrace::project(float x, float hgt, float z, float k, int *col, int *row) {
    double kk = (double)k * 0.01f;                   // 0x513d4c
    double dx = (double)x - camx;
    double dz = (double)z - camz;
    double dist = sqrt(dz * dz + dx * dx);
    double ang = atan2(dz, dx);
    if (ang < 0.0) ang += kTwoPi;
    int n = (int)angles.size();
    int c = view - (int)((double)n * ang / kTwoPi) - 320;
    int half = n / 2;
    if (c < -half) do c += n; while (c < -half);
    if (c > half) do c -= n; while (c > half);
    *col = c;
    int idx;
    if (c > -320 && c < 320) {
        if (c < 0) {
            int m = -(w / 2);
            if (c >= m) m = c;
            idx = m + n;
        } else {
            int m = w / 2 + c;
            if (!(c > m)) m = c;
            idx = m;
        }
    } else {
        idx = c < 0 ? c + n : c;
    }
    double cs = angles[idx].dx;
    if (cs == 0.0) cs = 1e-12;                       // 0x513dd8
    double ic = 1.0 / cs;
    double id = dist == 0.0 ? 1e-11 : 1.0 / dist;    // 0x513dd0 (as the exe: not its inverse)
    double p = (((double)160.0f + eyef) * 590.0f) * (id * ic);   // 0x513d1c, 0x513d24
    double q = 24000.0f * id * kk * ic * hgt * 0.01;   // 0x513d18, 0x513560
    *row = (int)(p - q);
}

// ---------------------------------------------------------------- drawing
// 0x41eae0 -> 0x40e680: a graphic drawn on its own (canvas mode switched as needed).
static void draw_graphic(Vision::Graphic *g) {
    if (!g->visible) return;
    Vision::canvas_set_mode(g->canvasMode());
    g->draw();
}

static double seg_len(double ax, double az, double bx, double bz) {
    double dx = ax - bx, dz = az - bz;
    return sqrt(dz * dz + dx * dx);
}

// 0x42d000: the view in 2 * rating slices; per slice, the wall clipped to the slice's edge rays, its ends projected
// and the current frame drawn on the trapezoid between them (the frame's strips cut where the wall was clipped).
void RayTrace::drawWall(WallObject *o) {
    TrapezoidRenderer r;
    if (!o->on) return;
    uint32_t now = Vision::frame_time();
    if (now - o->last >= o->frame_ms) {
        o->last = now;
        o->anim->step();
    }
    Line wl = { o->x1, o->z1, o->x2, o->z2 };
    int n = 2 * speedRating();
    double sd = 60.0f / (double)n;                   // 0x513dc8
    float slice = (float)sd;
    double hd = sd * 0.5f;
    float halfrad = (float)((double)kPi * hd * kDeg);
    float half = (float)hd;
    double len_dx = (double)wl.x2 - wl.x1, len_dz = (double)wl.z2 - wl.z1;
    for (int i = 0; i < n; i++) {
        double a = ((double)(i - n / 2) + 0.5f) * slice * kPi * kDeg + angf;
        float aL = (float)(a + halfrad), aR = (float)(a - halfrad);
        Line rayL = { camx, camz, (float)(cos((double)aL) * 100000.0 + camx),
                      (float)(sin((double)aL) * 100000.0 + camz) };   // 0x513dc0
        Line rayR = { camx, camz, (float)(cos((double)aR) * 100000.0 + camx),
                      (float)(sin((double)aR) * 100000.0 + camz) };
        // the ends' angles from the slice's middle, in degrees, wrapped to +-180
        float fdx = (float)((double)o->x1 - camx);
        double e = atan2((double)o->z1 - camz, (double)fdx) - a;
        if (e <= -(double)kPi) e += (double)kPi + kPi;
        else if (!(e < kPi)) e -= (double)kPi + kPi;
        float d1 = (float)(e * 180.0f / kPi);        // 0x513da4
        fdx = (float)((double)o->x2 - camx);
        e = atan2((double)o->z2 - camz, (double)fdx) - a;
        if (e <= -(double)kPi) e += (double)kPi + kPi;
        else if (!(e < kPi)) e -= (double)kPi + kPi;
        float d2 = (float)(e * 180.0f / kPi);
        r.skip_start = r.skip_end = 0;
        bool clip_right = false;
        r.setImage(o->anim->frame(o->anim->current()));
        if (!r.image) continue;                      // port: the exe would read through a null frame
        if (d1 < -half) continue;                    // wholly right of the slice
        if (!(d2 <= half)) continue;                 // wholly left of it
        float X = 0, Z = 0;
        double len = sqrt(len_dz * len_dz + len_dx * len_dx);
        if (!(d1 <= half)) {                         // the first end is past the left edge: cut there
            if (d2 < -half) clip_right = true;
            if (!wl.intersect(rayL, &X, &Z)) continue;
            int col;
            project(X, o->bottom, Z, o->k, &col, &r.c[2]);
            project(X, o->top, Z, o->k, &col, &r.c[3]);
            if (col == -319) col = -320;
            r.c[0] = col;
            float frac = (float)(seg_len(X, Z, o->x1, o->z1) / len);
            r.skip_start = (int)((double)r.image->oh * frac);
        } else {
            if (d2 < -half) clip_right = true;
            project(o->x1, o->bottom, o->z1, o->k, &r.c[0], &r.c[2]);
            project(o->x1, o->top, o->z1, o->k, &r.c[0], &r.c[3]);
        }
        if (clip_right) {                            // the second end is past the right edge
            if (!wl.intersect(rayR, &X, &Z)) continue;
            int col;
            project(X, o->bottom, Z, o->k, &col, &r.c[4]);
            project(X, o->top, Z, o->k, &col, &r.c[5]);
            if (col == 319) col = 320;
            r.c[1] = col;
            float frac = (float)(seg_len(X, Z, o->x2, o->z2) / len);
            r.skip_end = (int)((double)r.image->oh * frac);
        } else {
            project(o->x2, o->bottom, o->z2, o->k, &r.c[1], &r.c[4]);
            project(o->x2, o->top, o->z2, o->k, &r.c[1], &r.c[5]);
        }
        draw_graphic(&r);
    }
}

// 0x4265c0: every wall whose distance is in [lo, hi] (in the sorted order), each followed by its linked walls in
// falling key order.
void RayTrace::drawBand(double lo, double hi) {
    for (size_t i = 0; i < walls.size(); i++) {
        float d = order[i].dist;
        if (!(d <= hi) || d < lo) continue;
        WallObject *root = walls[order[i].idx];
        drawWall(root);
        int best = -1, bestd = -1;
        for (int j = 0; j < 10; j++) {
            WallObject *l = root->links[j];
            if (l && l->key < 1000000000) {
                int diff = 1000000000 - l->key;
                if (diff < bestd || best == -1) { bestd = diff; best = j; }
            }
        }
        if (best == -1) continue;
        for (WallObject *cur = root->links[best]; cur; cur = root->links[best]) {
            drawWall(cur);
            int key = cur->key;
            best = bestd = -1;
            for (int j = 0; j < 10; j++) {
                WallObject *l = root->links[j];
                if (l && l->key < key) {
                    int diff = key - l->key;
                    if (diff < bestd || best == -1) { bestd = diff; best = j; }
                }
            }
            if (best == -1) break;
        }
    }
}

static int by_distance(const void *a, const void *b) {   // 0x42cf00: far first
    float da = ((const RayTrace::Order *)a)->dist, db = ((const RayTrace::Order *)b)->dist;
    if (da < db) return 1;
    if (!(da <= db)) return -1;
    return 0;
}

// The end of 0x426710: each sprite's and wall's distance (a wall's: its farther end), both sorted far to near
// (qsort 0x4f54fc); sprites farther than every wall hit first, then each sprite after the walls from its distance
// out to the previous one's, then the walls nearer than the last.
// port: glibc's qsort may order equal distances differently from MSVC's.
void RayTrace::drawWalls() {
    sorder.resize(sprites.size());
    for (size_t i = 0; i < sprites.size(); i++) {
        Sprite3D *sp = sprites[i];
        double dx = (double)sp->x - camx, dz = (double)sp->z - camz;
        sp->dist = (float)sqrt(dx * dx + dz * dz);
        sorder[i].idx = (int)i;
        sorder[i].dist = sp->dist;
    }
    if (!sorder.empty()) qsort(sorder.data(), sorder.size(), sizeof(Order), by_distance);
    order.resize(walls.size());
    for (size_t i = 0; i < walls.size(); i++) {
        WallObject *o = walls[i];
        double ax = (double)o->x1 - camx, az = (double)o->z1 - camz;
        double d1 = sqrt(az * az + ax * ax);
        double bx = (double)o->x2 - camx, bz = (double)o->z2 - camz;
        float d2 = (float)sqrt(bz * bz + bx * bx);
        o->dist = d1 <= d2 ? d2 : (float)d1;
        order[i].idx = (int)i;
        order[i].dist = o->dist;
    }
    if (!order.empty()) qsort(order.data(), order.size(), sizeof(Order), by_distance);
    size_t i = 0;
    for (; i < sorder.size(); i++) {
        if (sorder[i].dist < maxdist) break;
        drawSprite(sorder[i].idx);
    }
    double hi = DBL_MAX;
    for (; i < sorder.size(); i++) {
        double lo = sorder[i].dist;
        drawBand(lo, hi);
        hi = lo;
        drawSprite(sorder[i].idx);
    }
    drawBand(0.0, hi);
}

}  // namespace Raytrace
