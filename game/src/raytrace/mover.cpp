// The curve and the camera mover (see mover.h).
#include "mover.h"
#include <string.h>

namespace Raytrace {

void Curve3::set0(const float v[3]) { memcpy(p0, v, sizeof p0); coeffs(); }
void Curve3::set1(const float v[3]) { memcpy(p1, v, sizeof p1); coeffs(); }
void Curve3::set2(const float v[3]) { memcpy(p2, v, sizeof p2); coeffs(); }
void Curve3::coeffs() {                              // 0x4010a0
    for (int i = 0; i < 3; i++) {
        double q = (double)p1[i] * 4.0f;             // 0x5132a0
        a[i] = (float)(q - (double)p0[i] * 3.0f - p2[i]);   // 0x51329c
        b[i] = (float)(((double)p0[i] + p2[i]) * 2.0 - q);
    }
}
void Curve3::at(float t, float out[3]) const {       // 0x401120
    double tt = (double)t * t;
    for (int i = 0; i < 3; i++) out[i] = (float)(tt * b[i] + (double)t * a[i] + p0[i]);
}

// 0x4316f0
void CameraMover::start(RayTrace *r, float x, float eye, float z, float deg, int m) {
    t0 = Vision::frame_time();
    ms = m;
    double a = r->angleDeg();
    a0 = (float)a;
    double d = (double)deg - a;
    da = (float)d;
    if (d > 180.0f) da = (float)(d - 360.0f);         // 0x513da4, 0x513de8
    if (da < -180.0f) da = (float)((double)da + 360.0f);   // 0x513de4
    float C[3];
    r->camPos(C);
    float T[3] = {x, eye, z}, D[3], H[3], M[3];
    for (int i = 0; i < 3; i++) D[i] = (float)((double)T[i] - C[i]);   // 0x425eb0
    for (int i = 0; i < 3; i++) H[i] = (float)((double)D[i] / 2.0f);   // 0x425f10
    for (int i = 0; i < 3; i++) M[i] = (float)((double)C[i] + H[i]);   // 0x425e50
    curve->set0(C);
    curve->set1(M);
    curve->set2(T);
    rt = r;
}

// 0x431890
bool CameraMover::step() {
    float t = (float)((double)(int64_t)(Vision::frame_time() - t0) / (double)ms);
    if (!(t < 1.0f)) t = 1.0f;
    float a = (float)((double)t * da + a0);
    float p[3];
    curve->at(t, p);
    rt->setCamera(p[0], p[1], p[2], a);
    return t < 1.0f;
}

// 0x4df200
void Tween::init(double f, double t, double total) {
    from = f;
    to = t;
    T = total;
    d1 = (t - f) * 0.5;                              // 0x542d78
    ta = 0.5 * total;
    a = 2.0 * d1 / (ta * ta);                        // 0x542d70
    v = a * ta;
    t0 = 0;
    running = true;
}

// 0x4df270
double Tween::value() {
    if (t0 == 0) {
        t0 = Vision::frame_time();
        return from;
    }
    double t = (double)(int64_t)(Vision::frame_time() - t0);
    if (t < ta) return 0.5 * a * t * t + from;
    if (!(t <= ta) && t < T) {
        double u = t - ta;
        return u * v + d1 + from - a * u * u * 0.5;
    }
    running = false;
    return to;
}

// 0x4df3d0
void CameraTween::start(RayTrace *r, float x, float eye, float z, float deg, double ms) {
    rt = r;
    tx.init(r->camx, x, ms);
    teye.init(r->eyef, eye, ms);
    tz.init(r->camz, z, ms);
    float d = (float)((double)deg - r->angleDeg());
    if (d > 180.0f) d = (float)((double)d - 360.0f);
    else if (d < -180.0f) d = (float)((double)d + 360.0f);
    tdeg.init(r->angleDeg(), r->angleDeg() + d, ms);
}

// 0x4df4d0
bool CameraTween::step() {
    bool all = tx.running && teye.running && tz.running && tdeg.running;
    float a = (float)tdeg.value();
    float z = (float)tz.value();
    float e = (float)teye.value();
    float x = (float)tx.value();
    rt->setCamera(x, e, z, a);
    return all;
}

}  // namespace Raytrace
