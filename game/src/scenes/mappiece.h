// A treasure map segment won by an area's puzzle game, flying in front of the camera, reconstructed from
// "Tinker Bell.exe": MapPiece (the shared part: ease 0x4e73f0, busy v15 0x4e73e0) and the Lost Boys' one,
// BucketMapPiece (vtable 0x517aac, ctor 0x4e8000, dtor 0x4545d0, load v11 0x4e8040, clear v12 0x4e8910,
// show v13 0x4e80f0, stop v14 0x4e80b0, run v10 0x4e8180): it rises out of the bucket tree, comes to 250 in
// front of the camera, and goes in a poof of sparkles.
#pragma once
#include "../vision/engine.h"
#include "../raytrace/raytrace.h"

namespace Game {

class MapPiece : public Vision::Task {
public:
    explicit MapPiece(Raytrace::RayTrace *r) : Task(nullptr), rt(r) {}
    virtual void load() = 0;                         // v11 +0x2c
    virtual void clear() { piece = poof = nullptr; } // v12 +0x30
    virtual void show() = 0;                         // v13 +0x34
    virtual void stop();                             // v14 +0x38
    bool busy() { return active(); }                 // v15 +0x3c 0x4e73e0 (the service's +0x14)
    bool ease(Raytrace::Sprite3D *s);                // 0x4e73f0: a fifth of the way to 250 ahead; false once there
    Raytrace::RayTrace *rt;                          // +0xc0
    bool fresh = false;                              // +0xc4: take the camera's place first
    float cx = 0, ceye = 0, cz = 0, cang = 0;        // +0xc8..+0xd4
    Raytrace::Sprite3D *piece = nullptr;             // +0xd8
    Raytrace::Sprite3D *poof = nullptr;              // +0xdc
};

class BucketMapPiece : public MapPiece {
public:
    explicit BucketMapPiece(Raytrace::RayTrace *r) : MapPiece(r) {}   // 0x4e8000
    void load() override;                            // v11 0x4e8040
    void show() override;                            // v13 0x4e80f0
    void run() override;                             // v10 0x4e8180
};

// Starry Skies' (vtable 0x517b34, ctor 0x4e8860, load v11 0x4e88a0, show v13 0x4e8920, run v10 0x4e89b0): Cloud
// Burst's segment appears by the animals' shelf ("interface3D"), comes to the camera and goes in a poof.
class StarryMapPiece : public MapPiece {
public:
    explicit StarryMapPiece(Raytrace::RayTrace *r) : MapPiece(r) {}   // 0x4e8860
    void load() override;                            // v11 0x4e88a0
    void show() override;                            // v13 0x4e8920
    void run() override;                             // v10 0x4e89b0
};

// Indian Village's (vtable 0x517af0, ctor 0x4e8410, load v11 0x4e8450, clear v12 0x4e8520, show v13 0x4e8540,
// stop v14 0x4e84e0, run v10 0x4e8610): Totem Trail's segment comes out of the stump as it opens. Its fields sit 4
// further on than the others' (+0xd8 the stump, +0xdc the segment, +0xe0 the poof).
class TotemMapPiece : public MapPiece {
public:
    explicit TotemMapPiece(Raytrace::RayTrace *r) : MapPiece(r) {}   // 0x4e8410
    void load() override;                            // v11 0x4e8450
    void clear() override { piece = poof = nullptr; stump = nullptr; }   // v12 0x4e8520
    void show() override;                            // v13 0x4e8540
    void run() override;                             // v10 0x4e8610
    Raytrace::Sprite3D *stump = nullptr;             // +0xd8 "AA_totm_ga_totstump_01"
};

// Mermaid Cove's (vtable 0x517a18, ctor 0x4e7500, load v11 0x4e7540, stop v14 0x4e75f0, clear v12 0x4e7640, show
// v13 0x4e7660, run v10 0x4e76f0): the Shell Game's segment comes out of the log as its lid opens and closes again.
// Its fields sit 4 further on than the others' (+0xd8 the lid, +0xdc the segment, +0xe0 the poof).
class ShellMapPiece : public MapPiece {
public:
    explicit ShellMapPiece(Raytrace::RayTrace *r) : MapPiece(r) {}   // 0x4e7500
    void load() override;                            // v11 0x4e7540
    void stop() override;                            // v14 0x4e75f0
    void clear() override { lid = nullptr; piece = poof = nullptr; }   // v12 0x4e7640
    void show() override;                            // v13 0x4e7660
    void run() override;                             // v10 0x4e76f0
    Raytrace::Attachment *lid = nullptr;             // +0xd8 "SHEL_GA_logopen_" on "AA_shel_ga_woodlog_01"
};

}  // namespace Game
