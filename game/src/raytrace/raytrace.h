// The game's 3D engine, "RayTrace" (a Vision::Screen subclass; RTTI "RayTrace"), reconstructed from "Tinker Bell.exe".
// A ray caster over wall *segments* (not a grid): one ray per screen column, the nearest wall it meets gives that
// column's texture column, distance and height. Ground and sky are drawn separately; sprites and walls are sorted
// by distance. Notes: docs/raytrace-notes.md. Work in progress: only what is reconstructed is here.
#pragma once
#include "../vision/engine.h"
#include "../vision/animation.h"
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

// A 2D line as the wall draw keeps it (x1, z1, x2, z2: four floats), with its helpers (0x42f850..0x42f9f0).
struct Line {
    float x1, z1, x2, z2;
    bool intercept(float *b) const;                 // 0x42f850: z at x = 0 (false for a vertical line)
    bool inX(float x) const;                        // 0x42f890: x within the line's x range (inclusive)
    bool inZ(float z) const;                        // 0x42f8f0
    bool zAt(float x, float *z) const;              // 0x42f950
    bool intersect(const Line &o, float *x, float *z) const;   // 0x42f9f0: where the two segments meet
};

// A textured wall drawn as a projected quad (0x42e5d0, 0xb4 bytes). One per textured wall of the .rtc, named
// "wall_<texture>_<NN>", its frames the animation named after the texture.
struct WallObject {
    WallObject(const char *name, const char *prefix);   // 0x42e5d0
    ~WallObject();
    bool loaded = false;                            // +0x00
    WallObject *links[10] = {};                     // +0x04..+0x28: walls drawn right after it, by key
    bool f2c = false;                               // +0x2c
    Vision::Animation *anim = nullptr;              // +0x30
    uint32_t last = 0;                              // +0x34: when the frame last stepped
    float dist = 0;                                 // +0x38: the farther end's distance (this frame)
    std::string name;                               // +0x3c
    std::string prefix;                             // +0x64
    float x1 = 0, bottom = 0, z1 = 0;               // +0x8c +0x90 +0x94
    float x2 = 0, top = 0, z2 = 0;                  // +0x98 +0x9c +0xa0 (top = the frame's width * 2.47)
    float k = 100.0f;                               // +0xa4: height percent
    int key = 0;                                    // +0xa8: draw order among linked walls
    uint32_t frame_ms = 50;                         // +0xac
    bool on = false;                                // +0xb0
};

// A graphic drawn with a sprite, offset and scaled with it (+0x148 prev, +0x14c next, +0x150 graphic, +0x17c on,
// +0x180 only with this frame of the sprite, -1 = any).
struct Attachment {
    Attachment *prev = nullptr, *next = nullptr;
    Vision::Graphic *g = nullptr;
    bool on = false;
    int frame = -1;
};

// A sprite standing in the 3D world (0x42e990, 0xb0 bytes): an animation at (x, z), y up, drawn scaled by distance.
struct Sprite3D {
    Sprite3D(const char *name, const char *prefix, int hit);   // 0x42e990
    ~Sprite3D();
    void load();                                    // 0x42ef20
    bool loaded = false;                            // +0x00
    std::vector<std::pair<std::string, Vision::Animation *>> anims;   // +0x04: by name (the main one first)
    float dist = 0;                                 // +0x14
    std::string name;                               // +0x18
    std::string prefix;                             // +0x40
    Attachment *attached = nullptr;                 // +0x68
    int hit = 1;                                    // +0x6c
    Vision::Animation *anim = nullptr;              // +0x70
    uint32_t last = 0;                              // +0x74
    float x = 0, y = 0, z = 0;                      // +0x78 +0x7c +0x80
    float scale = 100.0f;                           // +0x84 (percent)
    int mode = 1;                                   // +0x88 (2: attachments only on frame 0)
    float f8c = 90.0f;                              // +0x8c
    uint32_t f90 = 0, f94 = 0, f98 = 0, f9c = 0;    // +0x90..+0x9c
    bool box_flag = false;                          // +0xa0
    float radius = 0;                               // +0xa4: the collision box's half width
    float bheight = 0;                              // +0xa8: and its height
    uint32_t frame_ms = 50;                         // +0xac
};

// A collision box (0x44 bytes, list at RayTrace +0xa8): "<sprite>_box".
struct Box {
    std::string name;                               // +0x00 (0x28)
    float minx = 0, y0 = 0, minz = 0, maxx = 0, y1 = 0, maxz = 0;   // +0x28..+0x3c
};

class RayTrace : public Vision::Screen {
public:
    RayTrace();
    ~RayTrace() override;                           // port: frees what the loaders made (the exe's 0x429ff0 is not ported yet)
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
    void setCamera(float x, float eye, float z, float deg);   // 0x42a570 (deg: the view direction in degrees)
    void project(float x, float h, float z, float k, int *col, int *row);   // 0x42df80
    void drawWall(WallObject *o);                   // 0x42d000
    void drawBand(double lo, double hi);            // 0x4265c0: the walls whose distance is in [lo, hi]
    void drawWalls();                               // the end of 0x426710: walls and sprites by distance
    Sprite3D *addSprite(const char *name, const char *prefix, int hit);   // 0x42d9d0
    Sprite3D *findSprite(const char *name);         // 0x42dc70
    void makeBox(Sprite3D *s, bool flag);           // 0x429580 -> 0x429380
    void drawSprite(int i);                         // 0x42e120
    // 0x4aa570 (Raytrace_Scene's, on its RayTrace): data\<name>.rts, the sprites loaded from `pack`
    bool loadSprites(const char *name, Vision::Ras *pack);
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
    float angf = 0;                                 // +0x570: the angle at the screen's centre (radians)
    bool moved = false, ready = false;              // +0xa5 +0xa4
    std::vector<WallObject *> walls;                // +0x57c (count +0x578)
    struct Order { int idx; float dist; };
    std::vector<Order> order;                       // +0x964: walls by distance, far first
    std::vector<Sprite3D *> sprites;                // +0x1138
    std::vector<Order> sorder;                      // +0x1148: sprites by distance, far first
    std::vector<Box *> boxes;                       // +0xa8 (a linked list in the exe, in this order)
private:
    void fallback(Seg *start, Seg *end, float *x, float *z);   // 0x4279a0 (port: TODO)
};

// part of 0x428580: the wall object for a textured wall (walls.cpp)
void make_wall_object(RayTrace *rt, Vision::Pixelmap *tex, float x1, float z1, float x2, float z2);

}  // namespace Raytrace
