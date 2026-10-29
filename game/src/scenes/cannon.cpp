// The cannon timer (see cannon.h).
#include "cannon.h"

namespace Game {

using namespace Vision;

Cannon::Cannon()                                     // 0x4e3550
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      cannon("glob_if_cannon_01", 0, 0, 0, 0, 0, 0, true), bar("glob_if_cannonbar_01", 0, 0, 0, 0, 0, 0, true),
      smoke("glob_if_cansmoke_"), fuse("glob_if_fuse_"), spark("glob_if_spark_"),
      fuse_snd("GLOB_FX_fuse"), blast("GLOB_FX_blast") {
    z = -500;                                        // 0x411e70(0xfffffe0c)
}

void Cannon::load() {                                // v15 0x4e3780
    cannon.load();
    bar.load();
    fuse.load();
    spark.load();
    spark.start_time = frame_time();                 // 0x403be0(0, 2)
    spark.mode = 2;
    smoke.load();
    blast.open();                                    // 0x41f9d0(0)
    fuse_snd.mode = 1;                               // +0x874
    fuse_snd.open();
    fuse_snd.type = 6;                               // +0x87c: loops
    visible = false;
    fuse_snd.stop();
    blast.stop();
    lit = false;
}

void Cannon::unload() {                              // v16 0x4e3830
    Graphic::unload();
    cannon.unload();
    bar.unload();
    fuse.unload();
    spark.unload();
    smoke.unload();
    fuse_snd.close();
    blast.close();
}

void Cannon::draw() {                                // v14 0x4e38a0
    bar.draw();
    cannon.draw();
    Image *last = nullptr;
    for (int i = 0; i < pieces; i++) {
        last = fuse.frame(i);                        // 0x403ec0
        if (last) last->draw();
    }
    if (!lit) return;
    uint32_t now = real_time();                      // 0x4eca20
    uint32_t el = now - t0;
    if (el > 2000) fuse_snd.stop();
    uint32_t total = (uint32_t)secs * 1000;
    if (el <= total) {
        pieces = (int)((total - el) / 3000);
        if (now - spark_last > 0x28) {
            spark.step();                            // 0x403d60
            spark_last = now;
        }
        if (!last) last = fuse.frame(0);
        if (last) { spark.x = last->x; spark.y = last->y; }   // +0x58 +0x5c
        spark.draw();
        return;
    }
    smoke.draw();
    bool at_end = !(smoke.current() + 1 < smoke.count());
    if (!at_end) {
        if (!blasted) {
            blast.play();
            blasted = true;
        }
    } else if (blasted && !blast.isPlaying()) {
        blast.stop();
        lit = false;
    }
    if (now - smoke_last <= 0x28) return;
    smoke_last = now;
    smoke.step();
}

void Cannon::setTime(int s) {                        // 0x4e3ae0
    secs = s;
    int n = fuse.count();
    pieces = (int)((uint32_t)(secs * 1000) / 3000);
    if (n <= pieces) pieces = n - 1;
}

void Cannon::start(int s) {                          // 0x4e3a50
    setTime(s);
    restart();
}

void Cannon::restart() {                             // 0x4e3b30
    lit = true;
    fuse_snd.play();
    t0 = real_time();
    spark_last = t0;
    smoke_last = t0;
    smoke.setFrame(0);
    blasted = false;
}

bool Cannon::timeLeft() { return real_time() - t0 < (uint32_t)secs * 1000; }   // 0x4e3b90

void Cannon::stop() {                                // 0x4e3bc0
    fuse_snd.stop();
    blast.stop();
    lit = false;
}

void Cannon::hide() {                                // 0x4e3bf0
    visible = false;
    stop();
}

int Cannon::remaining() { return (int)(t0 + (uint32_t)secs * 1000 - real_time()); }   // 0x4e3c20

Cannon &cannon() { static Cannon *c = new Cannon(); return *c; }   // port: made on first use (state +0x7c0)

}  // namespace Game
