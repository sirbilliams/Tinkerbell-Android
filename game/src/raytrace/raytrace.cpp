// The 3D engine, reconstructed from "Tinker Bell.exe" (see raytrace.h and docs/raytrace-notes.md).
// The x87 code is followed from the disassembly where the decompiler lost operands; float vs double precision is
// kept as the exe stores it (float locals, double fields).
#include "raytrace.h"
#include <math.h>

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

}  // namespace Raytrace
