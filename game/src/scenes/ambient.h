// The ambient creatures that cross an area's sky, reconstructed from "Tinker Bell.exe":
//   AmbientSpriteTask (vtable 0x51769c, ctor 0x4df580, dtor 0x43dff0, run v10 0x4df720): every 2-4 s a new one,
//     one in eight carrying pixie dust (when the scene gave it the bag, +0xc4).
//   AmbientSprite@AmbientSpriteImpl (vtable 0x517724, 0x110 bytes, ctor 0x4df8d0, dtor 0x4dff30, run v10 0x4dfff0,
//     notify2 v9 0x4e0170): one of the area's creatures ("ambient_sprites.dat", the scene's section: prefix=air),
//     placed 700..10700 out to the left of the view, circling the camera until it leaves the view; a click on one
//     with dust gives the dust ("glob_fx_tnkhappy").
#pragma once
#include "../vision/engine.h"
#include "../raytrace/raytrace.h"
#include <string>

namespace Game {

class PixieDustBag;

class AmbientSprite : public Vision::Task {
public:
    AmbientSprite(Raytrace::RayTrace *rt, const std::string &scene, PixieDustBag *bag);   // 0x4df8d0
    ~AmbientSprite() override;                       // 0x4dff30
    void run() override;                             // v10 0x4dfff0
    void notify2(Vision::Service *s) override;       // v9 0x4e0170
    Raytrace::RayTrace *rt;                          // +0xc0
    std::string name;                                // +0xc4: "ambient_sprite_<n>"
    float speed = 0;                                 // +0xd4
    PixieDustBag *bag;                               // +0xd8: carries dust when set
    Vision::Mouse mouse;                             // +0xdc
};

class AmbientSpriteTask : public Vision::Task {
public:
    AmbientSpriteTask(Raytrace::RayTrace *r, const std::string &scene) : Task(nullptr), rt(r), scene(scene) {}   // 0x4df580
    void run() override;                             // v10 0x4df720
    Raytrace::RayTrace *rt;                          // +0xc0
    PixieDustBag *bag = nullptr;                     // +0xc4 (set by the scene)
    std::string scene;                               // +0xc8: the section
};

}  // namespace Game
