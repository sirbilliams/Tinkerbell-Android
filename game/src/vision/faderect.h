// Vision::FadeRect (vtable 0x5136c4): a graphic that darkens what is under its rectangle by a level 0..1
// (1 = untouched). ctor 0x415d30, level 0x415f20 (builds the 16-bit lookup table), draw 0x4160b0 (v14).
#pragma once
#include "engine.h"
#include <vector>

namespace Vision {
class FadeRect : public Graphic {
public:
    FadeRect();                                      // 0x415d30
    void setLevel(float level);                      // 0x415f20
    void draw() override;                            // 0x4160b0
    std::vector<uint16_t> table;                     // +0x98: RGB565 -> darkened RGB565 (0x10000 entries)
    float level = 1.0f;                              // +0xa0
};
}  // namespace Vision
