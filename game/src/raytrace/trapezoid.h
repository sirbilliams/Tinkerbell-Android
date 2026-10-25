// TrapezoidRenderer (vtable 0x513df0, a Graphic; ctor 0x4319a0, draw v14 0x431cc0): draws an image onto a vertical
// trapezoid - two vertical edges, each with its own bottom and top - the way the 3D engine draws a wall slice.
// The image is stored sideways: each image row is one vertical strip, texel 0 at the bottom.
//   0x431b10 setImage, 0x431b50 corners -> ints, 0x431c20 clip amounts, rasterizers by pixel format:
//   16-bit raw 0x431e00, 16-bit runs 0x4320b0 (kind 1) / 0x432c20 (kind 2),
//   8-bit palette raw 0x433970, runs 0x433c20 / 0x4347b0.
// port: the six rasterizers are two loops (raw, runs) with the pixel fetched through the palette when there is one;
// same stepping and arithmetic. Rows are found through a per-image row index instead of walking the runs from the
// start each time (same rows).
#pragma once
#include "../vision/engine.h"
#include <vector>

namespace Raytrace {

class TrapezoidRenderer : public Vision::Graphic {
public:
    TrapezoidRenderer();                             // 0x4319a0
    void setImage(Vision::Image *im);                // 0x431b10
    void draw() override;                            // 0x431cc0
    // the corners relative to (x, y), times scale / 100: left x, right x, left bottom, left top, right bottom,
    // right top (+0x140..+0x154)
    int c[6] = { -50, 50, 100, -100, 70, -70 };
    int skip_start = 0, skip_end = 0;                // +0x158 +0x15c: image rows left out at each end
    Vision::Image *image = nullptr;                  // +0x98
private:
    void corners();                                  // 0x431b50
    void clip();                                     // 0x431c20
    void drawRaw();                                  // 0x431e00 / 0x433970
    void drawRuns();                                 // 0x4320b0 / 0x432c20 / 0x433c20 / 0x4347b0
    const uint8_t *rowRuns(int row) const;
    uint16_t texel(const uint8_t *p) const;
    int x0 = 0, x1 = 0, lb = 0, lt = 0, rb = 0, rt = 0;   // +0x9c..+0xb0
    int pitch = 0;                                   // +0xb8 (pixels)
    int cw = 0, ch = 0;                              // +0xfc +0x100
    int clipT = 0, clipB = 0, clipL = 0, clipR = 0;  // +0x104 +0x108 +0x10c +0x110
    uint16_t *canvas = nullptr;                      // +0xc4
    int kind = 0;                                    // +0xcc
    const uint8_t *pix = nullptr;                    // +0xd0
    const uint16_t *pal = nullptr;                   // +0xf4
    int iw = 0, ih = 0;                              // +0x124 +0x128: the image's width (strip length) and height
    int bpp = 2;
};

}  // namespace Raytrace
