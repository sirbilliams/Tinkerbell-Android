// MapPiece and BucketMapPiece, reconstructed from "Tinker Bell.exe" (see mappiece.h).
#include "mappiece.h"
#include "../vision/animation.h"
#include <math.h>

namespace Game {

using Raytrace::Sprite3D;

static const float kPi = 3.14159274f;                // 0x517a0c

static void show_anim(Sprite3D *s, bool on) { if (s->anim) s->anim->visible = on; }   // +0x70 +0x7c

// 0x4e80b0
void MapPiece::stop() {
    if (!active()) return;                           // +0x14
    deactivate();                                    // +0x1c
    if (piece->anim) piece->anim->visible = false;
    if (poof->anim) poof->anim->visible = false;
}

// 0x4e73f0: toward the spot 250 in front of the camera as it was when asked (+0xc4), by a fifth of the way.
bool MapPiece::ease(Sprite3D *s) {
    if (fresh) {
        fresh = false;
        cx = rt->camx;                               // 0x42b470 (+0x568)
        ceye = (float)rt->eyeWorld();                // 0x42b520
        cz = rt->camz;                               // 0x42b540 (+0x56c)
        cang = rt->angf;                             // 0x42b550 (+0x570)
    }
    double dx = cos(cang) * 250.0 + cx - s->x;       // 0x5144a0
    double dy = (double)ceye - s->y;
    float dz = (float)(sin(cang) * 250.0 + cz - s->z);
    s->x = (float)(dx * 0.2f + s->x);                // 0x517a10
    s->y = (float)(dy * 0.2f + s->y);
    s->z = (float)(dz * 0.2f + s->z);
    if (!(fabs(dy) < 0.1)) return true;              // 0x513db8
    if (!(fabs(dx) < 0.1)) return true;
    if (!(fabs(dz) < 0.1)) return true;
    return false;
}

void BucketMapPiece::load() {                        // 0x4e8040
    piece = rt->addSprite("map_piece", "BUCK_GA_MAPseg", 1);   // 0x42d9d0
    piece->load();                                   // 0x42ef20
    if (piece->anim) piece->anim->visible = false;
    poof = rt->addSprite("poof", "glob_am_ambspark_", 1);
    poof->load();
    if (poof->anim) poof->anim->visible = false;
}

// 0x4e80f0: at the bucket tree (a step toward the view's direction), shown and started.
void BucketMapPiece::show() {
    Sprite3D *tree = rt->findSprite("bucket_tree");  // 0x42dc70
    piece->x = (float)(cos(rt->angf) + tree->x);
    piece->z = (float)(sin(rt->angf) + tree->z);
    piece->y = tree->y;
    piece->anim->start_time = Vision::frame_time();  // 0x403be0(0, 1)
    piece->anim->mode = 1;
    piece->anim->visible = true;
    reset();                                         // 0x421110
    activate();                                      // +0x18
}

// 0x4e8180: up 5 a step (and 5 to the side) until 300 high, every 30 ms; then to the camera; a second, the poof
// where it was (one step back toward the camera), a second, gone.
void BucketMapPiece::run() {
    wait(1);
    for (;;) {
        double side = (double)rt->angf + kPi * 0.5f; // 0x513298
        piece->x = (float)(cos(side) * 5.0 + piece->x);   // 0x515688
        piece->z = (float)(sin(side) * 5.0 + piece->z);
        piece->y = piece->y + 5.0f;                  // 0x514c84
        if (!(piece->y < 300.0f)) break;             // 0x515690
        wait(0x1e);
    }
    fresh = true;
    while (ease(piece)) wait(0x1e);
    float px = piece->x, py = piece->y, pz = piece->z;   // 0x42f700
    float bx = (float)((double)px - cos(rt->angf));
    float bz = (float)((double)pz - sin(rt->angf));
    poof->setPos(bx, py, bz);                        // 0x42f740
    poof->anim->visible = true;
    poof->anim->setFrame(0);
    poof->anim->start_time = Vision::frame_time();   // 0x403be0(0, 1)
    poof->anim->mode = 1;
    wait(1000);
    show_anim(piece, false);
    wait(1000);
    show_anim(poof, false);
    stop();                                          // v14
    finish();                                        // 0x420fe0
}

}  // namespace Game
