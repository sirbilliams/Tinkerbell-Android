// The game's 3D engine, "RayTrace" (a Vision::Screen subclass; RTTI "RayTrace"), reconstructed from "Tinker Bell.exe".
// A ray caster over wall *segments* (not a grid): one ray per screen column, the nearest wall it meets gives that
// column's texture column, distance and height. Ground and sky are drawn separately; sprites and walls are sorted
// by distance. Notes: docs/raytrace-notes.md. Work in progress: only what is reconstructed is here.
#pragma once
#include "../vision/engine.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace Raytrace {

// A wall: a 2D segment on the x/z plane (0x48 bytes, allocated by the loader 0x428580). 0x427560 fills the box
// and line; the loader the rest.
struct Seg {
    float minx = 0, minz = 0, maxx = 0, maxz = 0;   // +0x00..+0x0c: the bounding box
    float slope = 0;                                // +0x10: dz/dx
    float icpt = 0;                                 // +0x14: z at x = 0, or x for a vertical segment
    bool vertical = false;                          // +0x18: x1 == x2
    double angle = 0;                               // +0x20: atan2(..) - pi/2 (the loader)
    int tw = 0, th = 0;                             // +0x28 +0x2c: the texture's width and height (its sprite)
    uint16_t *tex = nullptr;                        // +0x30: the texture's pixels (null: an invisible wall)
    Seg *next = nullptr, *prev = nullptr;           // +0x34 +0x38: a circular list
    int base = 0;                                   // +0x3c: the height its bottom is at
    uint8_t flags = 0;                              // +0x40: bit 1 = x1 > x2, bit 0 = z1 > z2 (0x427560); ^3 = mirrored
};

// 0x427560: the box and line of the segment (x1, z1)-(x2, z2); returns the direction flags.
uint8_t seg_init(Seg *s, float x1, float z1, float x2, float z2);

// One screen column's hit (0x30 bytes each in the column buffer +0x548, two per cast).
struct Column {
    uint16_t *texcol = nullptr;                     // +0x00: the texture column (a row of the sideways texture)
    int col = 0;                                    // +0x04: the screen column
    int tw = 0;                                     // +0x08: the texture's width (the column's length)
    double dist = 0;                                // +0x10: distance along the view direction
    double scale = 0;                               // +0x18: 590 / dist, corrected for the column's angle
    int top = 0;                                    // +0x20: the screen row of the wall's top
    int topofs = 0;                                 // +0x24: top * screen width
    int base = 0;                                   // +0x28: the wall's base height
};

// The angle table (+0x528, +0x52c entries of 0x14 bytes): a full turn in screen-column steps.
struct Angle {
    float a = 0;                                    // +0x00: the angle (radians)
    float dx = 0;                                   // +0x04 (cos)
    float dz = 0;                                   // +0x08 (sin)
    int idx = 0, idz = 0;                           // +0x0c +0x10: dx, dz * 4096
};

class RayTrace : public Vision::Screen {
public:
    RayTrace();
    // 0x425f60: cast the ray `ray` (a segment from the camera) for screen column `col` against the walls from
    // `start` to `end` (null: start->prev); fills `out`, returns the wall hit (or the invisible one standing in).
    Seg *cast(int col, const Seg &ray, Seg *start, Column *out, Seg *end);
    // 0x426480: the columns strictly between lo and hi (already cast): the middle one against both wall lists, each
    // from the start given up to the hit of the neighbouring column, then both halves.
    void initAngles();                              // 0x427130
    bool loadWalls(const char *name);               // 0x428580 (+ 0x426f00): data\<name>.rtc
    bool loadGround(const char *path);              // 0x42cc80: the .grn (tile names + 128 x 128 grid)
    void groundTable();                             // 0x427620
    void drawSky();                                 // 0x42a140
    void drawGround2();                             // 0x42bf50 (the mode for a processor rating >= 4)
    void castColumns();                             // the column part of 0x426710
    static int speedRating();                       // 0x419170 / 0x4190c0
    void subdivide(unsigned lo, unsigned hi, Seg *startA, Seg *startB, Seg *endA, Seg *endB);
    Column *nextColumn() { return &columns[ncolumns++]; }   // +0x548[+0x54c++]

    // fields (offsets in the exe's object)
    int w = 640, h = 480;                           // +0x70 +0x74 (the Screen's size)
    int horizon = 0;                                // +0x90: h / 2 plus the pitch
    int eye = 0;                                    // +0x94: the camera's height
    double maxdist = 0;                             // +0x550: the farthest wall hit this frame
    double range = 0;                               // +0x540: the length of a ray
    std::vector<Column> columns;                    // +0x548: two per screen column (w * 0x60 bytes)
    int ncolumns = 0;                               // +0x54c
    std::vector<Angle> angles;                      // +0x528 / +0x52c
    // the wall textures (+0xb0 count, +0xb4 = their records, +0x4bc mirrored flags)
    std::vector<Vision::Pixelmap *> textures;
    std::vector<uint8_t> mirrored;
    int ground_tex = 0;                             // +0x4c0
    int sky_tex = 0;                                // +0x518
    int sky_angle = 0;                              // +0x520
    bool sky_clip = true;                           // +0x524
    int sky_rows = 0;                               // +0x51c
    Seg *listA = nullptr, *listB = nullptr;         // +0x534 +0x538
    std::vector<Seg *> segs;                        // port: owns the Seg allocations
    // the ground: +0x4f8/+0x4fc offset, the tile images (+0xb8/+0xbc), the 128 x 128 grid of tile pixels (+0x514)
    int ground_x = 0, ground_z = 0;
    std::vector<Vision::Image *> tiles;
    std::vector<const uint16_t *> grid;
    uint32_t g_umask = 0, g_vmask = 0, g_tmaskx = 0, g_tmasky = 0;   // +0x4f4 +0x4f0 +0x50c +0x508
    int g_vshift = 0, g_tshiftx = 0, g_tshifty = 0;                  // +0x4ec +0x504 +0x500
    float eyef = 0;                                 // +0xac: the camera height as set
    int eye_built = 0x75bcd15;                      // +0x9c: the height the table was built for
    std::vector<int> rowscale;                      // +0x98: (h/2 * 256) / (row + 1)
    std::vector<int> gtable;                        // +0x4d4: per half-screen column, h/2 distances
    int ground_mode = 2;                            // +0x4d8
    int ground_rows = 0;                            // +0x4e4: the rows the ground covers (its cache)
    int g_sky_top = 0;                              // 0x5a95a8: 240 - pitch / 8
    float camx = 0, camz = 0;                       // +0x568 +0x56c
    int view = 0;                                   // +0x574: the view direction (an index into `angles`)
private:
    void fallback(Seg *start, Seg *end, float *x, float *z);   // 0x4279a0 (port: TODO)
};

}  // namespace Raytrace
