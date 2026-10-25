// The 3D engine's sprites, reconstructed from "Tinker Bell.exe": animations standing in the world, from the
// scene's .rts, drawn scaled by distance between the bands of walls (see walls.cpp).
#include "raytrace.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "tb.h"
#include "../../../platform/dsound.h"   // tb_fopen_ci

namespace Vision { void canvas_set_mode(int m); }

namespace Raytrace {

static const float kTwoPi = 6.28318548f;            // 0x5a95b8

static void draw_graphic(Vision::Graphic *g) {       // 0x41eae0 -> 0x40e680
    if (!g->visible) return;
    Vision::canvas_set_mode(g->canvasMode());
    g->draw();
}

Sprite3D::Sprite3D(const char *n, const char *p, int h) : name(n), prefix(p), hit(h) {   // 0x42e990
    anim = new Vision::Animation(prefix.c_str());   // 0x404c20
    anims.push_back(std::make_pair(std::string(anim->name()), anim));
    anim->visible = false;
}
Sprite3D::~Sprite3D() {
    for (Attachment *a = attached; a;) { Attachment *n = a->next; delete a; a = n; }
    for (auto &e : anims) delete e.second;
}

void Sprite3D::load() {                              // 0x42ef20
    if (loaded) return;
    anim->load();                                    // v15
    anim->setHit(hit, false);                        // v6
    anim->mode = 1;                                  // +0x104: loop
    for (auto &e : anims) {                          // (the main one is in the map too)
        e.second->load();
        e.second->setHit(hit, false);
        e.second->mode = 1;
    }
    last = Vision::frame_time();
    loaded = true;
    anim->visible = true;
}

// 0x42efb0
void Sprite3D::setAnim(const char *p) {
    if (strcasecmp(prefix.c_str(), p) == 0) return;
    prefix = p;
    if (!loaded) return;
    bool vis = anim && anim->visible;
    int m = anim ? anim->mode : 1;
    Vision::Animation *a = nullptr;
    for (auto &e : anims) if (e.first == p) { a = e.second; break; }
    anim = a;
    if (!anim) {
        Vision::vision_log("Should preload %s into %s", p, name.c_str());   // 0x535ac8
        anim = new Vision::Animation(p);
        anims.push_back(std::make_pair(std::string(anim->name()), anim));
        anim->load();
        anim->setHit(hit, false);
        anim->mode = 1;
    }
    anim->mode = m;
    anim->setFrame(0);
    anim->visible = vis;
}

// 0x42f7f0
void Sprite3D::place() {
    if (!anim) return;
    int sum = 0, n = anim->count();
    for (int i = 0; i < n; i++) sum += anim->frame(i)->oh;
    if (n == 0) return;                              // port: the exe would divide by zero
    y = (float)((sum / n) / 2);
}

// 0x42da60
bool RayTrace::removeSprite(const char *n) {
    for (size_t i = 0; i < sprites.size(); i++)
        if (strcasecmp(sprites[i]->name.c_str(), n) == 0) {
            delete sprites[i];
            sprites.erase(sprites.begin() + (long)i);
            return true;
        }
    return false;
}

// 0x429600
bool RayTrace::removeBox(Sprite3D *s) {
    std::string bn = s->name + "_box";
    for (size_t i = 0; i < boxes.size(); i++)
        if (strcasecmp(boxes[i]->name.c_str(), bn.c_str()) == 0) {
            delete boxes[i];
            boxes.erase(boxes.begin() + (long)i);
            return true;
        }
    return false;
}

Sprite3D *RayTrace::addSprite(const char *n, const char *p, int h) {   // 0x42d9d0
    sprites.push_back(new Sprite3D(n, p, h));
    return sprites.back();
}

Sprite3D *RayTrace::findSprite(const char *n) {      // 0x42dc70
    for (Sprite3D *s : sprites)
        if (strcasecmp(s->name.c_str(), n) == 0) return s;
    return nullptr;
}

// 0x429580 -> 0x429380: "<name>_box", the sprite's square (half width +0xa4) and its height band in the units of the
// eye height: (scale * y / 100 -+ height * 1.35 / 2 - 480) * 0.4878.
void RayTrace::makeBox(Sprite3D *s, bool flag) {
    double hh = (double)s->bheight * 1.35f * 0.5f;   // 0x513d50, 0x513298
    double yc = (double)s->scale * s->y * 0.01f;     // 0x513d4c
    Box *b = new Box();
    b->y0 = (float)((yc - hh - 480.0f) * 0.487804890f);   // 0x513d48, 0x513d44
    b->y1 = (float)((yc + hh - 480.0f) * 0.487804890f);
    float r = s->radius;
    b->minx = (float)((double)s->x - r);
    b->maxx = (float)((double)r + s->x);
    b->minz = (float)((double)s->z - r);
    b->maxz = (float)((double)r + s->z);
    b->name = s->name + "_box";
    boxes.push_back(b);
    s->box_flag = flag;
}

static bool rd32(FILE *f, void *p) { return fread(p, 4, 1, f) == 1; }

// 0x4aa570: header 0x58, u32 count, per sprite: name[40], animation[40] (trailing digits dropped), then
// +0x88 mode, x, y, z, scale, +0x8c, radius, height, frame ms, +0x90..+0x9c. Then each is loaded from `pack`
// (made current for it) and given its box.
bool RayTrace::loadSprites(const char *nm, Vision::Ras *pack) {
    char path[300];
    snprintf(path, sizeof path, "data\\%s.rts", nm);   // 0x53fefc
    FILE *f = tb_fopen_ci(path);
    if (!f) return false;
    fseek(f, 0x58, SEEK_SET);
    uint32_t n = 0;
    rd32(f, &n);
    for (uint32_t i = 0; i < n; i++) {
        char name[41] = {0}, an[41] = {0};
        if (fread(name, 40, 1, f) != 1 || fread(an, 40, 1, f) != 1) break;
        int k = (int)strlen(an) - 1;                 // drop the trailing frame number
        if (k >= 0 && an[k] >= '0') {
            bool any = false;
            for (;;) {
                if (an[k] > '9' || k == 0) {
                    if (any) an[k + 1] = 0;
                    break;
                }
                char c = an[k - 1];
                k--;
                any = true;
                if (c < '0') { an[k + 1] = 0; break; }
            }
        }
        addSprite(name, an, 1);
        Sprite3D *s = findSprite(name);
        rd32(f, &s->mode);
        rd32(f, &s->x); rd32(f, &s->y); rd32(f, &s->z); rd32(f, &s->scale);
        rd32(f, &s->f8c);
        rd32(f, &s->radius); rd32(f, &s->bheight); rd32(f, &s->frame_ms);
        rd32(f, &s->f90); rd32(f, &s->f94); rd32(f, &s->f98); rd32(f, &s->f9c);
    }
    fclose(f);
    Vision::Ras *prev = Vision::ras_set_current(pack);
    for (uint32_t i = 0; i < n && i < sprites.size(); i++) {
        sprites[i]->load();
        makeBox(sprites[i], false);
    }
    Vision::ras_set_current(prev);
    return true;
}

// 0x42f760: the attachments bubble-sorted by their graphics' z (+0x60), larger first.
static void sort_attached(Sprite3D *s) {
    for (;;) {
        bool swapped = false;
        Attachment *a = s->attached;
        if (!a) return;
        for (Attachment *b = a->next; b; b = a->next) {
            if (a->g->z < b->g->z) {
                if (a == s->attached) s->attached = b;
                if (a->prev) a->prev->next = b;
                if (b->next) b->next->prev = a;
                swapped = true;
                b->prev = a->prev;
                a->prev = b;
                a->next = b->next;
                b->next = a;
            } else {
                a = b;
            }
        }
        if (!swapped) return;
    }
}

// 0x42e120: the sprite's column (from its angle, as 0x42df80) and row, its animation scaled by 24000 / distance.
void RayTrace::drawSprite(int i) {
    Sprite3D *s = sprites[(size_t)i];
    Vision::Animation *an = s->anim;
    if (!an || !an->visible) return;
    float k = (float)((double)s->scale * 0.01f);
    double ang = atan2((double)s->z - camz, (double)s->x - camx);
    if (ang < 0.0) ang += kTwoPi;
    int n = (int)angles.size();
    int col = view - (int)((double)n * ang / kTwoPi) - 320;
    if (col < -(n / 2)) col += n;
    else if (col > n / 2) col -= n;
    if (!(col > -w && col < w)) return;
    if (s->dist == 0.0f) s->dist = 1e-5f;            // 0x3727c5ac
    int idx;
    if (col < 0) {
        idx = -(w / 2);
        if (col >= idx) idx = col;
        idx += n;
    } else {
        idx = w / 2;
        if (!(col > idx)) idx = col;
    }
    float cs = angles[(size_t)idx].dx;
    double sc = 24000.0f / s->dist * k / cs;         // 0x513d18
    double p = ((double)160.0f + eyef) * 590.0f / s->dist / cs;
    an->x = col;
    an->y = (int)(p - (double)s->y * sc * 0.01);
    if (getenv("TB_DEBUG_SPRITE") && s->name == getenv("TB_DEBUG_SPRITE"))
        tb_log("sprite %s at %.0f,%.0f,%.0f dist %.0f col %d row %d scale %.1f frames %d vis %d", s->name.c_str(),
               s->x, s->y, s->z, s->dist, an->x, an->y, sc, an->count(), (int)an->visible);
    an->setScale(sc);
    draw_graphic(an);
    if (s->mode == 2 && an->current() != 0) return;
    sort_attached(s);
    for (Attachment *a = s->attached; a; a = a->next) {
        if (!a->on) continue;
        if (a->frame != -1 && a->frame != an->current()) continue;
        a->g->setScale(sc);
        int gx = a->g->x, gy = a->g->y;
        a->g->x = (int)((double)gx * sc * 0.01 + an->x);
        a->g->y = (int)((double)gy * sc * 0.01 + an->y);
        draw_graphic(a->g);
        a->g->x = gx;
        a->g->y = gy;
    }
}

}  // namespace Raytrace
