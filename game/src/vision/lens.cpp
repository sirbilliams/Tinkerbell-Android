// Vision::Lens, reconstructed from "Tinker Bell.exe" (see lens.h).
#include "lens.h"

namespace Vision {

Lens::Lens(int x_, int y_, int z_, int r)            // 0x414dd0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu), radius(r) {
    setZ(z_);                                        // (+0x60 then the layer's sort)
    x = x_;
    y = y_;
    buf.assign((size_t)r * r, 0);                    // new(r * r * 2)
}

// 0x414f80: only for a 16-bit display (else vision_abort). Nothing when the circle is wholly off the canvas;
// when it reaches an edge, v18 (0x4150b0); else v19 (0x415350).
// port: v19 zooms in place on the canvas, two rows at a time from the centre outward, each row's span from
// sqrt(r^2 - dy^2); it is drawn here with v18's buffered copy, which gives the same doubled picture (the exe
// keeps the second path only because it skips the copy).
void Lens::draw() {
    Canvas *c = current_canvas();                    // 0x41eaa0(&w, &h)
    if (!c || !c->pixels) return;
    cw = c->w;
    ch = c->h;
    pixels = c->pixels;
    pitch = c->pitch;                                // 0x5a73ec (bytes there)
    int r = radius;
    int hh = ch / 2, hw = cw / 2;
    if (y + r <= -hh || y - r >= hh || x + r <= -hw || x - r >= hw) return;
    drawEdge();
}

// 0x4150b0: copy the r x r square centred on x, y (clipped; the rest left black) into the buffer, then write it
// back doubled over the 2r x 2r square, only where (i - r)^2 + (j - r)^2 <= r^2 and on the canvas.
void Lens::drawEdge() {
    int r = radius;
    std::fill(buf.begin(), buf.end(), (uint16_t)0);
    int top = ch / 2 - r / 2 + y;
    int left = cw / 2 - r / 2 + x;
    int skip_t = top < 0 ? -top : 0;
    int skip_b = (top + r - 1 >= ch) ? top + r - 1 - ch + 1 : 0;
    int skip_l = left < 0 ? -left : 0;
    int skip_r = (left + r - 1 >= cw) ? left + r - 1 - cw + 1 : 0;
    int rows = r - skip_b - skip_t;
    int cols = r - skip_r - skip_l;
    if (cols > 0 && rows > 0) {
        uint16_t *d = buf.data() + r * skip_t + skip_l;
        const uint16_t *s = pixels + (skip_t + top) * pitch + left + skip_l;
        for (int j = 0; j < rows; j++, d += r, s += pitch)
            std::copy(s, s + cols, d);
    }
    int oy = ch / 2 - r + y;
    int ox = cw / 2 - r + x;
    int rr = r * r;
    for (int j = 0; j < 2 * r; j++) {
        int yy = oy + j;
        if (yy < 0 || yy >= ch) continue;
        const uint16_t *src = buf.data() + (j / 2) * r;
        uint16_t *dst = pixels + yy * pitch + ox;
        int dy2 = (j - r) * (j - r);
        int step = 0;
        for (int i = 0; i < 2 * r; i++) {
            int dx = i - r;
            if (dx * dx + dy2 <= rr && ox + i >= 0 && ox + i < cw) *dst = *src;
            dst++;
            src += step;
            step = 1 - step;
        }
    }
}

}  // namespace Vision
