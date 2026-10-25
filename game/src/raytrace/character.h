// RayTraceCharacter (vtable 0x517330, ctor 0x4cc5c0): something in the 3D world that is one named sprite of the
// RayTrace whose animation it switches (Tink, Peter, Wendy...). Reconstructed from "Tinker Bell.exe".
#pragma once
#include "raytrace.h"
#include <string>

namespace Raytrace {

class RayTraceCharacter {
public:
    RayTraceCharacter(RayTrace *rt, const char *name);   // 0x4cc5c0
    virtual ~RayTraceCharacter();                    // 0x4cc710
    virtual void init() {}                           // v1 (0x4ef760: nothing)
    virtual void release();                          // v2 0x4cc750: the sprite (and its box) out of the world
    virtual void notify(Vision::Service *) {}        // v3

    // 0x4cc780: the sprite shows the animation <prefix> (made, loaded and placed on first use; `pack` made current
    // first when given), its frame mode (sprite +0x88) and play mode (animation +0x104).
    Sprite3D *setAnim(const char *prefix, Vision::Ras *pack, int mode, int loop);
    void noBox();                                    // 0x4cc880
    Sprite3D *sprite();                              // 0x4cc8b0
    void setX(float v);                              // 0x4cc8c0
    void setY(float v);                              // 0x4cc910
    void setZ(float v);                              // 0x4cc960
    float getX() const { return x; }                 // 0x4cc9b0
    float getY() const { return y; }                 // 0x4cc9c0
    float getZ() const { return z; }                 // 0x4cc9d0
    void refresh();                                  // 0x4cc9e0
    void hide();                                     // 0x4cca40
    void show();                                     // 0x4cca70

    RayTrace *rt;                                    // +0x04
    float box_h = 150.0f, box_r = 150.0f;            // +0x08 +0x0c
    bool has_box = true;                             // +0x10
    float x = 0, y = 0, z = 0;                       // +0x14 +0x18 +0x1c
    bool visible = true;                             // +0x20
    std::string name;                                // +0x21
    int hit = 1;                                     // +0x4c
};

}  // namespace Raytrace
