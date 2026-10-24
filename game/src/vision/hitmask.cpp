// Pixel-exact hit tests for Images: Graphic::setHit (0x411e90) builds the hit mask from the sprite, and
// Graphic::hit (+0x2c, 0x411ee0) asks the mask (0x418910) whether the point is on an opaque pixel.
//
// port: the exe keeps the mask as per-row runs (row starts at +0xc, run lengths at +0x10, transparent run
// first, w/h at +4/+8) and walks them; the port keeps one byte per pixel. Same answers, including the scaled
// case (the mask is sampled at row*mh/h, col*mw/w as 0x418910 does) and the four rotations.
#include "engine.h"
#include <string.h>

namespace Vision {

namespace {
uint32_t rd32(const std::vector<uint8_t> &d, size_t o) {
    return o + 4 <= d.size() ? (uint32_t)d[o] | (uint32_t)d[o + 1] << 8 | (uint32_t)d[o + 2] << 16 | (uint32_t)d[o + 3] << 24 : 0;
}
}

// Opaque-pixel mask of a sprite record, decoded the way the blitter reads it (docs/formats.md, kinds 0..3).
bool build_hit_mask(const Pixelmap *pm, std::vector<uint8_t> &mask, int &mw, int &mh) {
    mw = pm->w; mh = pm->h;
    if (mw <= 0 || mh <= 0) return false;
    mask.assign((size_t)mw * mh, 0);
    const std::vector<uint8_t> &d = pm->data;
    size_t o = pm->pixels;
    int bp = pm->bpp == 8 ? 1 : 2;
    int kind = pm->kind & ~0x10;
    if (kind == 0) {                                  // raw: every pixel is part of the image
        memset(&mask[0], 1, mask.size());
        return true;
    }
    if (kind == 3) {                                  // byte stream: escape E; E,value,n = run; value E = transparent
        if (o >= d.size()) return false;
        uint8_t esc = d[o++];
        size_t i = 0, n = mask.size();
        while (i < n && o < d.size()) {
            uint8_t v, b = d[o];
            uint32_t cnt;
            if (b == esc) { if (o + 2 >= d.size()) break; v = d[o + 1]; cnt = d[o + 2]; o += 3; }
            else { v = b; cnt = 1; o++; }
            if (v != esc) for (uint32_t k = 0; k < cnt && i + k < n; k++) mask[i + k] = 1;
            i += cnt;
        }
        return true;
    }
    for (int y = 0; y < mh; y++) {                    // kinds 1/2: per row, transparent count / opaque count + pixels
        int x = 0;
        bool opaque = false;
        while (x < mw && o < d.size()) {
            uint32_t cnt = d[o++];
            if (kind == 2 && cnt == 0xff) { if (o + 2 > d.size()) return true; cnt = d[o] | d[o + 1] << 8; o += 2; }
            if (opaque) {
                for (uint32_t k = 0; k < cnt && x + (int)k < mw; k++) mask[(size_t)y * mw + x + k] = 1;
                o += (size_t)cnt * bp;
            }
            x += (int)cnt;
            opaque = !opaque;
        }
    }
    return true;
}

}  // namespace Vision

namespace Vision {

bool build_hit_mask(const Pixelmap *pm, std::vector<uint8_t> &mask, int &mw, int &mh);

bool Image::buildHitMask(std::vector<uint8_t> &m, int &mw, int &mh) {
    return pixelmap && build_hit_mask(pixelmap, m, mw, mh);
}

void Graphic::setHit(int mode, bool f) {             // 0x411e90
    f90 = f;
    hit_mask.clear(); has_mask = false;               // the old mask is freed (0x4188e0)
    hitmode = mode;
    if (mode > 0) has_mask = buildHitMask(hit_mask, hit_mw, hit_mh);   // 0x4122a0(mode - 1, f8c)
}

// 0x418910: is (px, py), relative to the screen, on an opaque pixel of this graphic?
static bool mask_hit(Graphic *g, int px, int py) {
    int u, v;                                         // column, row in the graphic
    switch (g->rotation) {
    case 0:   u = g->w / 2 - g->x + px; v = g->h / 2 - g->y + py; break;
    case 180: u = g->w / 2 + g->x - px; v = g->h / 2 + g->y - py; break;
    case 90:  v = g->h / 2 + g->x - px; u = g->w / 2 - g->y + py; break;
    case 270: u = g->w / 2 + g->y - py; v = g->h / 2 - g->x + px; break;
    default:  return false;                           // port: other angles fall through with garbage in the exe
    }
    if (u < 0 || u >= g->w || v < 0 || v >= g->h) return false;
    int col = u, row = v;
    if (g->scale != 100.0) {                          // *(double *)0x5134c0 (port: 100.0, the unscaled value)
        row = g->hit_mh * v / g->h; if (row >= g->hit_mh) row = g->hit_mh - 1;
        col = g->hit_mw * u / g->w; if (col >= g->hit_mw) col = g->hit_mw - 1;
    }
    if (col < 0 || row < 0 || col >= g->hit_mw || row >= g->hit_mh) return false;
    return g->hit_mask[(size_t)row * g->hit_mw + col] != 0;
}

bool Graphic::hit(int px, int py) {                  // +0x2c 0x411f90
    if (f90 && !visible) return false;
    if (hitmode == 0) return false;
    if (has_mask) {
        if (layer) return mask_hit(this, px - layer->screen->x, py - layer->screen->y);
        return mask_hit(this, px, py);
    }
    return inBox(px, py);
}

}  // namespace Vision
