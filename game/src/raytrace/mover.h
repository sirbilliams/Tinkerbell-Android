// Two small helpers of the 3D scenes, reconstructed from "Tinker Bell.exe": the quadratic curve (0x401000..)
// that characters and the camera fly along, and the camera mover (0x431600) that flies the camera to a spot.
#pragma once
#include "raytrace.h"

namespace Raytrace {

// 0x401000..0x401120 (0x3c bytes): the quadratic through p0 (t = 0), p1 (t = 0.5) and p2 (t = 1).
struct Curve3 {
    float p0[3] = {}, p1[3] = {}, p2[3] = {};        // +0x00 +0x0c +0x18
    float a[3] = {}, b[3] = {};                      // +0x24.. interleaved in the exe: (a, b) per axis
    void set0(const float v[3]);                     // 0x401040
    void set1(const float v[3]);                     // 0x401060
    void set2(const float v[3]);                     // 0x401080
    void coeffs();                                   // 0x4010a0
    void at(float t, float out[3]) const;            // 0x401120
};

// 0x431600 (0x18 bytes): the camera from where it is to (x, eye, z) along a curve through the half-way point,
// turning by the shorter way to `deg`, in `ms` milliseconds.
struct CameraMover {
    CameraMover() : curve(new Curve3()) {}           // 0x431600
    ~CameraMover() { delete curve; }                 // 0x431670
    void start(RayTrace *rt, float x, float eye, float z, float deg, int ms);   // 0x4316f0
    bool step();                                     // 0x431890: true while not there yet
    RayTrace *rt = nullptr;                          // +0x00
    Curve3 *curve;                                   // +0x04
    float a0 = 0, da = 0;                            // +0x08 +0x0c: the start angle and the turn
    uint32_t t0 = 0;                                 // +0x10
    int ms = 0;                                      // +0x14
};

// 0x4df200..0x4df270 (0x40 bytes): from `from` to `to` in T ms, speeding up for the first half (0.5 a t^2)
// and slowing down for the second.
struct Tween {
    void init(double from, double to, double T);     // 0x4df200
    double value();                                  // 0x4df270: the first call starts the clock
    double from = 0, to = 0, T = 0;                  // +0x00 +0x08 +0x10
    double d1 = 0, ta = 0, a = 0, v = 0;             // +0x18 +0x20 +0x28 +0x30: half the way, half the time, accel, top speed
    uint32_t t0 = 0;                                 // +0x38
    bool running = false;                            // +0x3c
};

// 0x4df3d0 / 0x4df4d0 (TinkerbellRaytraceScene +0x16c0): the camera to (x, eye, z, deg) by four tweens.
struct CameraTween {
    void start(RayTrace *rt, float x, float eye, float z, float deg, double ms);   // 0x4df3d0
    bool step();                                     // 0x4df4d0: true while all four still run (checked first)
    RayTrace *rt = nullptr;                          // +0x00
    Tween tx, teye, tz, tdeg;                        // +0x08 +0x48 +0x88 +0xc8
};

}  // namespace Raytrace
