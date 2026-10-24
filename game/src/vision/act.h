// Vision::Act (vtable 0x513e8c, ctor/dtor 0x435be0): a Task with two extra virtuals, +0x30 (v12) and +0x34 (v13),
// that the game's "acts" (DelayedVO, Inactivity*, Instruction, PauseOver) implement: start watching / stop.
// No fields of its own. (Reconstructed by lane B for DelayedVO; the other acts derive from it too.)
#pragma once
#include "engine.h"

namespace Vision {

class Act : public Task {
public:
    explicit Act(const char *name = nullptr) : Task(name) {}   // 0x421260 then vtable 0x513e8c
    virtual void start() = 0;        // +0x30
    virtual void stop() = 0;         // +0x34
};

}  // namespace Vision
