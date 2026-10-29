// The 3D engine's camera movement and collision, reconstructed from "Tinker Bell.exe": a boundary polygon 150 units
// inside the walls (built once at load), point-in-polygon tests by counting crossings of a ray towards +z, sprite
// boxes, barrier lines, and sliding along the wall that stopped a move.
// x87 precision as in raytrace.cpp (values kept on the FPU are doubles, stores to float fields round).
#include "raytrace.h"
#include <math.h>
#include "tb.h"

namespace Raytrace {

static const float kPi = 3.14159274f;               // 0x513d28
static const float kTwoPi = 6.28318548f;            // 0x5a95b8
static const float kEyeBox = 117.073174f;           // 0x513d54: eye height to box units
static const double kNoAngle = 1.7976931348623157e308;   // 0x513da8: "no wall found"
static double g_last_slide = 0;                     // 0x5a9610: the last wall slid along

static int round_(double v) { return (int)floor(v + 0.5); }   // 0x425b50

// Keeps a coordinate off whole units (|v - round(v)| < 0.01 -> round(v) + 0.01), as every mover does.
static void nudge(float &v) {
    int r = round_(v);
    if (fabs((double)v - r) < 0.01) v = (float)((double)r + 0.01f);
}

// The ray (x, z) -> (x, z + 2 * range) against one segment: does it cross?
static bool ray_crosses(const Seg &r, const Seg *e) {
    if (r.minz > e->maxz || r.minx > e->maxx || r.maxx < e->minx) return false;
    if (e->vertical) return e->icpt == r.icpt;
    double zz = (double)r.icpt * e->slope + e->icpt;
    return !(zz < r.minz) && zz <= r.maxz;
}

// 0x427760: `out` = `src` moved `dist` along the direction `ang`, as a line `range` long each way through the moved
// midpoint (vertical walls move sideways).
void RayTrace::offsetSeg(Seg *src, Seg *out, int dist, double ang) {
    if (src->vertical) {
        out->vertical = true;
        if (!((double)kPi * 0.5f <= ang) && (double)kPi * -0.5f < ang)
            out->icpt = (float)((double)dist + src->icpt);
        else
            out->icpt = (float)((double)src->icpt - dist);
        out->minx = out->icpt;
        out->maxx = out->icpt;
        double mz = ((double)src->maxz + src->minz) * 0.5f;
        out->maxz = (float)(range + mz);
        out->minz = (float)(mz - range);
        out->angle = src->angle;
        return;
    }
    unsigned n = (unsigned)angles.size();
    double mx = ((double)src->maxx + src->minx) * 0.5f;
    double mz = ((double)src->maxz + src->minz) * 0.5f;
    float nf = (float)(double)n;
    float angf_ = (float)ang;
    auto idx = [&](int i) { while (i < 0) i += (int)n; return (unsigned)i % n; };
    int i1 = (int)(ang * nf / kTwoPi);
    float distf = (float)dist;
    double px = (double)angles[idx(i1)].dx * distf + mx;
    double pz = (double)angles[idx(i1)].dz * distf + mz;
    float hpi = (float)((double)kPi * 0.5f);
    int i2 = (int)(((double)angf_ + hpi) * nf / kTwoPi);
    double x1 = (double)angles[idx(i2)].dx * range + px;
    int i3 = (int)(((double)angf_ - hpi) * nf / kTwoPi);
    double x2 = (double)angles[idx(i3)].dx * range + px;
    double z1 = (double)angles[idx(i2)].dz * range + pz;
    double z2 = (double)angles[idx(i3)].dz * range + pz;
    seg_init(out, (float)x1, (float)z1, (float)x2, (float)z2);
    out->angle = src->angle;
}

// 0x427a80: the wall after `s` for the boundary: the next one of its list if a point just inside it (moved 10
// along its normal, plus 1) is in the room; else the other list's first.
Seg *RayTrace::nextBoundary(Seg *s) {
    std::vector<Box *> saved;
    saved.swap(boxes);                               // the boxes are left out of the test
    Seg *nx = s->next;
    if (nx->base == 0) {
        Seg t;
        offsetSeg(nx, &t, 10, nx->angle);
        float px = (float)(((double)t.minx + t.maxx) * 0.5f + 1.0f);   // 0x513710
        float pz = (float)(((double)t.minz + t.maxz) * 0.5f + 1.0f);
        if (pointInside(px, pz)) {
            boxes.swap(saved);
            return nx;
        }
    }
    Seg *e = listA;
    Seg *last = listA->prev;
    for (;;) {
        if (e == nx) { e = listB; break; }
        if (e == last) break;
        e = e->next;
    }
    if (e != listB) e = listA;
    float fx, fz;
    fallback(nx, s, &fx, &fz);                       // (its result is not used)
    boxes.swap(saved);
    return e->next;
}

// 0x427f00: where two segments meet (overlapping collinear ones: the middle of the overlap).
static bool seg_meet(const Seg *a, const Seg *b, float *x, float *z) {
    if (!(a->minx <= b->maxx) || a->maxx < b->minx || !(a->minz <= b->maxz) || a->maxz < b->minz) return false;
    if (a->vertical) {
        if (b->vertical) {
            if (b->icpt != a->icpt) return false;
            *x = a->icpt;
            *z = a->minz;                            // (the exe works out the overlap's middle, then overwrites it)
            return true;
        }
        float zz = (float)((double)b->slope * a->icpt + b->icpt);
        if (zz < a->minz || !(zz <= a->maxz)) return false;
        *z = zz;
        *x = a->icpt;
        return true;
    }
    if (b->vertical) {
        float zz = (float)((double)a->slope * b->icpt + a->icpt);
        if (zz < b->minz || !(zz <= b->maxz)) return false;
        *z = zz;
        *x = b->icpt;
        return true;
    }
    double ds = (double)a->slope - b->slope;
    if (ds != 0.0) {
        float xi = (float)(((double)b->icpt - a->icpt) / ds);
        if (xi < a->minx || !(xi <= a->maxx) || xi < b->minx || !(xi <= b->maxx)) return false;
        *z = (float)((double)xi * a->slope + a->icpt);
        *x = xi;
        return true;
    }
    if (a->icpt != b->icpt) return false;
    if (a->minx <= b->minx) {
        if (a->maxx < b->maxx) {
            *x = (float)(((double)a->maxx + b->minx) * 0.5f);
            *z = (float)(((double)a->maxz + b->minz) * 0.5f);
        } else {
            *x = (float)(((double)b->minx + b->maxx) * 0.5f);
            *z = (float)(((double)b->maxz + b->minz) * 0.5f);
        }
    } else if (a->maxx < b->maxx) {
        *x = (float)(((double)a->minx + a->maxx) * 0.5f);
        *z = (float)(((double)a->maxz + a->minz) * 0.5f);
    } else {
        *x = (float)(((double)a->minx + b->maxx) * 0.5f);
        *z = (float)(((double)b->maxz + a->minz) * 0.5f);
    }
    return true;
}

static bool same_line(const Seg *a, const Seg *b) {   // both vertical, or the same slope
    if (a->vertical) return b->vertical;
    if (b->vertical) return false;
    return a->slope == b->slope;
}

// 0x427b70: the boundary: every wall (walking 0x427a80) moved 150 inwards, collinear neighbours merged, each cut
// where it meets its neighbours. A circular list (next/prev).
Seg *RayTrace::buildBoundary() {
    Seg *first = new Seg();
    bsegs.push_back(first);
    Seg *cur = first;
    Seg *s = nextBoundary(listA);
    Seg *start = s;
    for (;;) {
        offsetSeg(s, cur, 150, s->angle);
        s = nextBoundary(s);
        if (s == start) break;
        Seg *n = new Seg();
        bsegs.push_back(n);
        cur->next = n;
        cur = n;
    }
    cur->next = first;
    Seg *prev = cur;
    Seg *e = first;
    do { e->prev = prev; prev = e; e = e->next; } while (e != first);
    e = first;
    while (same_line(e, e->next)) e = e->next;
    Seg *head = e->next;
    float px = 0, pz = 0;                            // port: the exe leaves these unset when the two don't meet
    seg_meet(head, head->prev, &px, &pz);
    float sx = px, sz = pz, ex = 0, ez = 0;
    Seg *a = head;
    do {
        for (;;) {                                   // merge the collinear ones after it
            Seg *n = a->next;
            if (!same_line(a, n) || n == head) break;
            a->next = n->next;
            n->next->prev = a;
            for (size_t i = 0; i < bsegs.size(); i++)
                if (bsegs[i] == n) { bsegs.erase(bsegs.begin() + (long)i); break; }
            delete n;
        }
        if (a->next == head) { ex = sx; ez = sz; }
        else seg_meet(a, a->next, &ex, &ez);
        seg_init(a, ex, ez, px, pz);
        a = a->next;
        px = ex;
        pz = ez;
    } while (a != head);
    return head;
}

// 0x42ba80: (x, z) inside both wall lists (an odd number of crossings each) and in no box.
bool RayTrace::pointInside(float x, float z) {
    Seg r;
    seg_init(&r, x, z, x, (float)(range + range + z));
    for (int l = 0; l < 2; l++) {
        Seg *head = l == 0 ? listA : listB;
        Seg *last = head->prev;
        int c = 0;
        for (Seg *e = head;; e = e->next) {
            if (ray_crosses(r, e)) c++;
            if (e == last) break;
        }
        if (!(c & 1)) return false;
    }
    for (Box *b : boxes)
        if (!(x < b->minx) && x <= b->maxx && !(z < b->minz) && z <= b->maxz) return false;
    return true;
}

static int boundary_crossings(const Seg &r, Seg *head) {
    int c = 0;
    Seg *last = head->prev;
    for (Seg *e = head;; e = e->next) {
        if (ray_crosses(r, e)) c++;
        if (e == last) break;
    }
    return c;
}

Box *RayTrace::boxAt(float x, float z) {             // the box (x, z) is in at the eye's height
    float y = (float)((double)eyef - kEyeBox);
    for (Box *b : boxes)
        if (!(x < b->minx) && x <= b->maxx && !(z < b->minz) && z <= b->maxz && !(y < b->y0) && y <= b->y1)
            return b;
    return nullptr;
}

// 0x42b580: may the camera step to (x, z)? Not when that is outside the boundary (then +0x560 = the angle of the
// boundary segment crossed on the way) or inside a box (+0x560 = the side it was met from).
bool RayTrace::canMove(float x, float z) {
    Seg r;
    seg_init(&r, x, z, x, (float)(range + range + z));
    if (boundary_crossings(r, boundary) & 1) {
        Box *b = boxAt(x, z);
        if (!b) return true;
        if (camz < b->minz) slide = (double)kPi * -0.5f;   // 0x513290
        else if (!(camz <= b->maxz)) slide = (double)kPi * 0.5f;
        else if (camx < b->minx) slide = kPi;
        else slide = 0.0;
        g_last_slide = slide;
        return false;
    }
    Seg m;
    seg_init(&m, camx, camz, x, z);
    double found = kNoAngle;
    Seg *e = boundary;
    do {
        bool hit = false;
        if (!(m.minx > e->maxx) && !(m.maxx < e->minx) && !(m.minz > e->maxz) && !(m.maxz < e->minz)) {
            if (m.vertical) {
                if (e->vertical) hit = e->icpt == m.icpt;
                else {
                    double zz = (double)m.icpt * e->slope + e->icpt;
                    hit = !(zz < m.minz) && zz <= m.maxz;
                }
            } else if (e->vertical) {
                double zz = (double)m.slope * e->icpt + m.icpt;
                hit = !(zz < e->minz) && zz <= e->maxz;
            } else {
                double ds = (double)m.slope - e->slope;
                if (ds == 0.0) hit = m.icpt == e->icpt;
                else {
                    float xi = (float)(((double)e->icpt - m.icpt) / ds);
                    hit = !(xi < m.minx) && xi <= m.maxx && !(xi < e->minx) && xi <= e->maxx;
                }
            }
        }
        if (hit) { found = e->angle; break; }
        e = e->next;
    } while (e != boundary);
    slide = found;
    if (slide == kNoAngle) slide = g_last_slide;
    g_last_slide = slide;
    return false;
}

// 0x42b910: the same test for a slide, without noting a wall.
bool RayTrace::canSlide(float x, float z) {
    if (!boundary) return true;
    Seg r;
    seg_init(&r, x, z, x, (float)(range + range + z));
    if (!(boundary_crossings(r, boundary) & 1)) return false;
    return boxAt(x, z) == nullptr;
}

// 0x42d700: does the move (x1, z1) -> (x2, z2) at height y cross a barrier line (+0x1158)?
bool RayTrace::barrierHit(float x1, float z1, float x2, float z2, float y) {
    Line m = { x1, z1, x2, z2 };
    for (Barrier *b : barriers) {
        if (y < b->y0 || !(y <= b->y1)) continue;
        float x, z;
        if (b->line.intersect(m, &x, &z)) return true;
    }
    return false;
}

static unsigned wrap_index(int i, unsigned n) {
    while (i < 0) i += (int)n;
    return (unsigned)i % n;
}

// The slide after a blocked step: along the stopping wall, the way nearer the view (`back`: nearer the reverse).
bool RayTrace::slideAlong(float dist, bool back) {
    float h = (float)((double)kPi * 0.5f);
    double a1 = slide - h;
    double a2 = (double)h + slide;
    double e1, e2;
    if (back) { e1 = fabs((a1 - angf) + kPi); e2 = fabs((a2 - angf) + kPi); }
    else { e1 = fabs(a1 - angf); e2 = fabs(a2 - angf); }
    while (e1 > kTwoPi) e1 -= kTwoPi;
    while (e2 > kTwoPi) e2 -= kTwoPi;
    if (e1 > kPi) e1 = kTwoPi - e1;
    if (e2 > kPi) e2 = kTwoPi - e2;
    unsigned n = (unsigned)angles.size();
    double dir = e1 < e2 ? a1 : a2;
    int i = (int)(dir * (int)n / kTwoPi);
    float dx = (float)((double)angles[wrap_index(i, n)].dx * dist);
    float dz = (float)((double)angles[wrap_index(i, n)].dz * dist);
    float nx = (float)((double)dx + camx), nz = (float)((double)dz + camz);
    if (!canSlide(nx, nz)) return false;
    if (barrierHit(camx, camz, nx, nz, eyef)) return false;
    camx = (float)((double)dx + camx);
    moved = true;
    camz = (float)((double)dz + camz);
    nudge(camx);
    nudge(camz);
    return true;
}

// 0x42a890: a step of `dist` in the view direction; if a wall or box is in the way, a slide along it.
void RayTrace::moveForward(float dist) {
    unsigned n = (unsigned)angles.size();
    int i = (int)((double)(int64_t)n * angf / kTwoPi);
    float dx = (float)((double)angles[wrap_index(i, n)].dx * dist);
    float dz = (float)((double)angles[wrap_index(i, n)].dz * dist);
    bool ok = true;
    if (!no_walls) ok = canMove((float)((double)dx + camx), (float)((double)dz + camz));
    if (ok && !barrierHit(camx, camz, (float)((double)camx + dx), (float)((double)camz + dz), eyef)) {
        camx = (float)((double)dx + camx);
        moved = true;
        camz = (float)((double)dz + camz);
        nudge(camx);
        nudge(camz);
    } else {
        slideAlong(dist, false);
    }
    if (moved) {
        if (move_start == 0) move_start = Vision::frame_time();
        move_last = Vision::frame_time();
    } else {
        move_start = 0;
    }
}

// 0x42ace0: a step back (no walking timestamps).
void RayTrace::moveBack(float dist) {
    unsigned n = (unsigned)angles.size();
    move_start = 0;
    int i = (int)((double)(int64_t)n * angf / kTwoPi);
    float dx = (float)((double)angles[wrap_index(i, n)].dx * dist);
    float dz = (float)((double)angles[wrap_index(i, n)].dz * dist);
    bool ok = true;
    if (!no_walls) ok = canMove((float)((double)camx - dx), (float)((double)camz - dz));
    if (ok && !barrierHit(camx, camz, (float)((double)camx - dx), (float)((double)camz - dz), eyef)) {
        camx = (float)((double)camx - dx);
        moved = true;
        camz = (float)((double)camz - dz);
        nudge(camx);
        nudge(camz);
        return;
    }
    slideAlong(dist, true);
}

// 0x535a28: the highest eye, a global the scenes change (Lost: 291); read from the exe until one does.
static int g_max_eye = -1;
int RayTrace::maxEye() { if (g_max_eye < 0) g_max_eye = tb_exe_i32(0x535a28); return g_max_eye; }
void RayTrace::setMaxEye(int v) { g_max_eye = v; }          // 0x42b110
void RayTrace::resetMaxEye() { g_max_eye = 0x1c2; }         // 0x42b120

// 0x42b130 / 0x42b2d0: the eye up / down by `d`, kept in 0..max (0x535a28) and out of boxes (undone if it would
// enter one).
void RayTrace::raise(float d) {
    eyef = (float)((double)d + eyef);
    if (eyef < 0.0f) eyef = 0.0f;
    float mx = (float)maxEye();
    if (eyef > mx) eyef = mx;
    nudge(eyef);
    if (boxAt(camx, camz)) eyef = (float)((double)eyef - d);
    else { moved = true; groundTable(); }
    nudge(eyef);
}
void RayTrace::lower(float d) {
    eyef = (float)((double)eyef - d);
    if (eyef < 0.0f) eyef = 0.0f;
    float mx = (float)maxEye();
    if (eyef > mx) eyef = mx;
    nudge(eyef);
    if (boxAt(camx, camz)) eyef = (float)((double)d + eyef);
    else { moved = true; groundTable(); }
    nudge(eyef);
}

// 0x42a790: turn by `deg` degrees.
void RayTrace::turn(float deg) {
    moved = true;
    angf = (float)((double)kPi * deg * 0.00555555569f + angf);
    while (angf < 0.0f) angf = (float)((double)kTwoPi + angf);
    while (!(angf < kTwoPi)) angf = (float)((double)angf - kTwoPi);
    int n = (int)angles.size();
    // the left edge (+30 degrees, 0x513d98 = 1/6) as the view index
    view = (int)((double)(int64_t)n * ((double)kPi * 0.166666672f + angf) / kTwoPi);
    while (view < 0) view += n;
    while (n <= view) view -= n;
}

// The end of 0x4281a0: with a textured wall in either list, the boundary (else the camera goes anywhere).
void RayTrace::initCollision() {
    bool found = false;
    for (int l = 0; l < 2 && !found; l++) {
        Seg *h = l == 0 ? listA : listB;
        if (!h) continue;
        Seg *e = h;
        do { if (e->tex) found = true; e = e->next; } while (!found && e != h);
    }
    no_walls = !found;
    boundary = found ? buildBoundary() : nullptr;
}

// 0x4272e0: the ray length: the largest distance between corners of the textured walls (the last of each list
// left out, as the exe's loops stop there), plus 1.
double RayTrace::wallRange() {
    double best = 0.0;
    Seg *head = listA;
    Seg *e = head;
    if (head->next == head) return sqrt(best) + 1.0;
    int corner = 1;
    float cx = 0, cz = 0;
    for (;;) {
        Seg *nx = e->next;
        if (e->tex) {
            switch (corner) {
            case 1: cx = e->maxx; cz = e->maxz; break;
            case 2: cz = e->minz; break;
            case 3: cx = e->minx; break;
            default: cz = e->maxz; break;
            }
            for (int l = 0; l < 2; l++) {
                Seg *h = l == 0 ? listA : listB;
                if (!h) continue;
                for (Seg *o = h; o->next != h; o = o->next) {
                    if (!o->tex) continue;
                    float dz1 = (float)((double)o->maxz - cz);
                    double dx1 = (double)o->maxx - cx;
                    float sx1 = (float)(dx1 * dx1);
                    float v = (float)((double)dz1 * dz1 + sx1);
                    if (best < (double)dz1 * dz1 + sx1) best = v;
                    double dz2 = (double)o->minz - cz;
                    float sz2 = (float)(dz2 * dz2);
                    v = (float)((double)sx1 + sz2);
                    if (best < (double)sx1 + sz2) best = v;
                    double dx2 = (double)o->minx - cx;
                    float sx2 = (float)(dx2 * dx2);
                    v = (float)((double)sx2 + sz2);
                    if (best < (double)sx2 + sz2) best = v;
                    v = (float)((double)dz1 * dz1 + sx2);
                    if (best < (double)dz1 * dz1 + sx2) best = v;
                }
            }
            if (corner < 4) { corner++; }
            else { e = nx; corner = 1; }
        } else {
            e = nx;
        }
        if (e->next == head) break;
    }
    return sqrt(best) + 1.0;
}

}  // namespace Raytrace
