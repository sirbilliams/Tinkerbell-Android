// Vision::FadeRect, reconstructed from "Tinker Bell.exe" (see faderect.h).
#include "faderect.h"

namespace Vision {

FadeRect::FadeRect() : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {}   // 0x415d30

// 0x415f20: clamp to 0..1, then per RGB565 value: each channel widened to 8 bits (blue << 3, green << 2,
// red << 3, from the display masks 0x5a67e8/ec/f4 and shifts 0x5a65c4/0x5a67d0/0x5a65c8), times k >> 16, narrowed.
// port: k = (int)(level * 65536): the exe gets it from the runtime's float-to-int (0x4f3948, in the encrypted
// pages); the 8-bit-display path (0x513998 == 0) is not needed (the port always runs the 16-bit path).
void FadeRect::setLevel(float v) {
    if (v < 0.0f) level = 0.0f;
    else if (v > 1.0f) level = 1.0f;
    else level = v;
    int k = (int)(level * 65536.0f);
    if (table.empty()) table.resize(0x10000);
    for (uint32_t c = 0; c < 0x10000; c++) {
        uint32_t b = (c & 0x1f) << 3, g = ((c & 0x7e0) >> 5) << 2, r = ((c & 0xf800) >> 11) << 3;
        b = (b * (uint32_t)k) >> 16; g = (g * (uint32_t)k) >> 16; r = (r * (uint32_t)k) >> 16;
        table[c] = (uint16_t)(((b >> 3) & 0x1f) | (((g >> 2) << 5) & 0x7e0) | (((r >> 3) << 11) & 0xf800));
    }
}

// 0x4160b0: nothing at level 1; else every pixel of the rectangle (centred on x, y; clipped) through the table.
void FadeRect::draw() {
    if (level == 1.0f || table.empty()) return;
    Canvas *c = current_canvas();
    if (!c || !c->pixels) return;
    int x0 = (x - w / 2) + c->w / 2, x1 = w + x0;
    int y0 = (y - h / 2) + c->h / 2, y1 = h + y0;
    if (x0 < 0) x0 = 0;
    if (c->w <= x1) x1 = c->w - 1;
    if (y0 < 0) y0 = 0;
    if (c->h <= y1) y1 = c->h - 1;
    for (int yy = y0; yy <= y1; yy++) {
        uint16_t *p = c->pixels + yy * c->pitch + x0;
        for (int xx = x0; xx <= x1; xx++, p++) *p = table[*p];
    }
}

}  // namespace Vision
