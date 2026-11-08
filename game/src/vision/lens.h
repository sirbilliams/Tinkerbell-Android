// Vision::Lens (vtable 0x513624): a magnifying glass. It doubles the size of what is drawn under a circle of the
// given radius around its x, y. ctor 0x414dd0 (x, y, z, radius), dtor 0x414ea0, draw 0x414f80 (v14),
// near an edge 0x4150b0 (v18), inside the screen 0x415350 (v19).
#pragma once
#include "engine.h"
#include <vector>

namespace Vision {
class Lens : public Graphic {
public:
    Lens(int x, int y, int z, int radius);           // 0x414dd0
    void draw() override;                            // v14 0x414f80
    void drawEdge();                                 // v18 0x4150b0
    int pitch = 0;                                   // +0x98
    int cw = 0, ch = 0;                              // +0x9c +0xa0: the canvas
    int radius;                                      // +0xa4
    uint16_t *pixels = nullptr;                      // +0xa8
    std::vector<uint16_t> buf;                       // +0xac: radius * radius pixels
};
}  // namespace Vision
