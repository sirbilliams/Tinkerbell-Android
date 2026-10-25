// TrapezoidRenderer, reconstructed from "Tinker Bell.exe" (see trapezoid.h).
#include "trapezoid.h"

namespace Raytrace {
using namespace Vision;

TrapezoidRenderer::TrapezoidRenderer()               // 0x4319a0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu) {}

void TrapezoidRenderer::setImage(Image *im) {        // 0x431b10
    image = im;
    kind = im && im->pixelmap ? im->pixelmap->kind : 0;
    if (((uint32_t)kind >> 4 & 1) == 0 && im) { iw = im->ow; ih = im->oh; }
}

// 0x431b50: corner = c * scale * 0.01 + position + half the canvas (x87 doubles, then truncated).
void TrapezoidRenderer::corners() {
    double s = scale * 0.01;
    double hx = (double)(cw / 2), hy = (double)(ch / 2);
    x0 = (int)((double)c[0] * s + (double)x + hx);
    x1 = (int)((double)c[1] * s + (double)x + hx);
    lb = (int)((double)c[2] * s + (double)y + hy);
    lt = (int)((double)c[3] * s + (double)y + hy);
    rb = (int)((double)c[4] * s + (double)y + hy);
    rt = (int)((double)c[5] * s + (double)y + hy);
}

void TrapezoidRenderer::clip() {                     // 0x431c20
    int b = lb <= rb ? rb : lb;
    clipB = ch < b ? b - ch : 0;
    int t = lt <= rt ? lt : rt;
    clipT = t < 0 ? -t : 0;
    clipL = x0 < 0 ? -x0 : 0;
    clipR = cw < x1 ? x1 - cw : 0;
}

uint16_t TrapezoidRenderer::texel(const uint8_t *p) const {
    return pal ? pal[*p] : (uint16_t)(p[0] | p[1] << 8);
}

// The start of image row `row`'s runs (kinds 1 and 2: per row, transparent count, opaque count, its pixels;
// kind 2 writes a count of 255 or more as 0xff + u16).
const uint8_t *TrapezoidRenderer::rowRuns(int row) const {
    const uint8_t *p = pix;
    for (int r = 0; r < row; r++) {
        int left = iw;
        while (left > 0) {
            int t = *p++;
            if ((kind & ~0x10) == 2 && t == 0xff) { t = p[0] | p[1] << 8; p += 2; }
            left -= t;
            if (left <= 0) break;
            int o = *p++;
            if ((kind & ~0x10) == 2 && o == 0xff) { o = p[0] | p[1] << 8; p += 2; }
            left -= o;
            p += o * bpp;
        }
    }
    return p;
}

void TrapezoidRenderer::draw() {                     // 0x431cc0
    Canvas *cv = current_canvas();
    if (!cv || !image || !image->pixelmap) return;
    pitch = cv->pitch;
    cw = cv->w;
    ch = cv->h;
    canvas = cv->pixels;
    Pixelmap *pm = image->pixelmap;
    pix = pm->data.data() + pm->pixels;
    pal = image->palette ? (const uint16_t *)(image->palette->data.data() + image->palette->entries) : nullptr;
    bpp = pal ? 1 : 2;
    corners();
    clip();
    int width = x1 - x0;
    if (clipR + clipL < width) {
        int k = kind & ~0x10;                        // port: the 0x10 (alpha) flag isn't in the exe's switch
        if (k == 0) drawRaw();
        else if (k == 1 || k == 2) drawRuns();
    }
}

void TrapezoidRenderer::drawRaw() {                  // 0x431e00 (16-bit) / 0x433970 (palette)
    int width = x1 - x0;
    if (width < 1) return;
    uint32_t acc = 0;                                // +0x130 (14-bit fraction)
    int i = 0;                                       // +0x114
    int step = (ih << 14) / width;                   // +0x134: image rows per screen column
    int row = 0;                                     // +0xe8 as a row number
    if (skip_start == 0) {
        if (skip_end != 0) step = ((ih - skip_end) << 14) / width;
    } else {
        row = skip_start;
        step = ((ih - skip_start) * 0x4000) / width;
        if (skip_end != 0) step = ((ih - skip_end - skip_start) << 14) / width;
    }
    if (clipL != 0) {
        uint32_t v = (uint32_t)(clipL * step);
        i = clipL;
        row += (int)v >> 14;
        acc = v & 0x3fff;
    }
    int end = width - clipR;                         // +0x118
    for (; i < end; i++) {
        int yb = lb + ((rb - lb) * i) / width;       // +0x120
        int yt = (lt + ((rt - lt) * i) / width);     // +0x11c
        if (yb <= yt) return;
        int vstep = (iw << 14) / (yb - yt) - 1;      // +0x13c
        int s = 0;                                   // +0x138
        int y = yb;
        if (ch <= yb) {                              // below the canvas: start at its last row
            int cut = yb - ch + 1;
            y = yb - cut;
            s = vstep * cut;
        }
        int ystop = yt < 1 ? 0 : yt;                 // ((yt < 1) - 1) & yt
        const uint8_t *src = pix + (size_t)row * iw * bpp;
        for (; y >= ystop; y--) {
            if (y < ch && x0 + i >= 0 && x0 + i < cw)    // port: the exe has no check here
                canvas[(size_t)y * pitch + x0 + i] = texel(src + (size_t)(s >> 14) * bpp);
            s += vstep;
        }
        uint32_t v = (uint32_t)step + acc;
        acc = v & 0x3fff;
        row += (int)v >> 14;
    }
}

// 0x4320b0 (and its siblings): per screen column, the image row's runs; a transparent run moves up the column
// without drawing, an opaque run draws its stretch.
void TrapezoidRenderer::drawRuns() {
    int width = x1 - x0;
    if (width < 1) return;
    int step = (ih << 14) / width;
    int row = 0;
    uint32_t acc = 0;
    if (skip_start == 0) {
        if (skip_end != 0) step = ((ih - skip_end) << 14) / width;
    } else {
        row = skip_start;
        step = ((ih - skip_start) * 0x4000) / width;
        if (skip_end != 0) step = ((ih - skip_end - skip_start) << 14) / width;
    }
    int i = 0;
    if (clipL != 0) {
        uint32_t v = acc + (uint32_t)(step * clipL);
        row += (int)v >> 14;
        i = clipL;
        acc = v & 0x3fff;
    }
    int end = width - clipR;
    for (; i < end; i++) {
        int yb = ((rb - lb) * i) / width + lb;
        int yt = ((rt - lt) * i) / width + lt;
        int span = yb - yt;
        if (span <= 0) return;                       // port: the exe would divide by zero / draw nothing useful
        int vstep = (iw << 14) / span - 1;           // +0x13c: image texels per screen pixel
        int dstep = (span * 0x4000) / iw - 1;        // screen pixels per image texel
        int y = yb;
        uint32_t a = 0;
        const uint8_t *p = rowRuns(row);
        int left = iw;
        bool k2 = (kind & ~0x10) == 2;
        while (left != 0) {
            int t = *p++;
            if (k2 && t == 0xff) { t = p[0] | p[1] << 8; p += 2; }
            if (t != 0) {
                left -= t;
                if (left == 0) break;
                a += (uint32_t)t * dstep;
                y -= (int)a >> 14;
                a &= 0x3fff;
            }
            int o = *p++;
            if (k2 && o == 0xff) { o = p[0] | p[1] << 8; p += 2; }
            if (o != 0) {
                left -= o;
                uint32_t a2 = a + (uint32_t)o * dstep;
                int ystop = y - ((int)a2 >> 14);
                a = a2 & 0x3fff;
                const uint8_t *src = p;
                uint32_t s = 0;
                while (ystop < y) {
                    if (y >= 0 && y < ch && x0 + i >= 0 && x0 + i < cw)   // port: the exe draws unclipped here
                        canvas[(size_t)y * pitch + x0 + i] = texel(src);
                    y--;
                    s += vstep;
                    src += (s >> 14) * bpp;
                    s &= 0x3fff;
                }
                p += o * bpp;
            }
            if (left < 0) break;                     // port: guards a malformed row
        }
        uint32_t v = acc + (uint32_t)step;
        acc = v & 0x3fff;
        row += (int)v >> 14;
    }
}

}  // namespace Raytrace
