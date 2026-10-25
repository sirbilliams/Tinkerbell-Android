// Desktop test aid (not in the game): TB_TEST_RT=<scene> renders that 3D scene's walls/ground/sky from a fixed
// camera, TB_RT_CAM="x,z,degrees,eye" (set through RayTrace::setCamera). Start with `tinkerbell rttest`.
#include "raytrace.h"
#include "tb.h"
#include <stdlib.h>
#include <stdio.h>

namespace {
using namespace Vision;

struct TestRT : Raytrace::RayTrace {
    float walk = 0, spin = 0;                        // TB_RT_WALK="step,turn" per frame
    void draw() override {
        if (walk > 0) moveForward(walk);
        else if (walk < 0) moveBack(-walk);
        if (spin != 0) turn(spin);
        if (walk != 0 || spin != 0) tb_log("rttest: cam %.2f,%.2f angle %.3f", camx, camz, angf);
        RayTrace::draw();
    }
};

struct RtTestScene : Scene {
    TestRT rt;
    std::string scene;
    Ras *walls = nullptr, *ground = nullptr, *sprites = nullptr;
    explicit RtTestScene(const char *s) : Scene("rttest"), scene(s) {}
    void enter() override {
        walls = ras_open((scene + "_walls").c_str(), "", &data_device());
        walls->load();
        ras_set_current(walls);
        sprites = ras_open((scene + "_sprites").c_str(), "", &data_device());
        sprites->load();
        ground = ras_open((scene + "_ground").c_str(), "", &data_device());
        ground->load();
        rt.load(scene.c_str());
        tb_log("rttest: range %.1f, boundary %zu segments", rt.range, rt.bsegs.size());
        rt.loadSprites(scene.c_str(), sprites);
        tb_log("rttest: %zu sprites", rt.sprites.size());
        rt.loadGround(("data\\" + scene + ".grn").c_str());
        float minx = 1e9f, minz = 1e9f, maxx = -1e9f, maxz = -1e9f;
        for (size_t i = 0; i < rt.segs.size(); i++) {
            Raytrace::Seg *s = rt.segs[i];
            if (s->minx < minx) minx = s->minx;
            if (s->minz < minz) minz = s->minz;
            if (s->maxx > maxx) maxx = s->maxx;
            if (s->maxz > maxz) maxz = s->maxz;
        }
        tb_log("rttest: %zu walls, box %.0f,%.0f - %.0f,%.0f; ground offset %d,%d; %zu tiles", rt.segs.size(),
               minx, minz, maxx, maxz, rt.ground_x, rt.ground_z, rt.tiles.size());
        tb_log("rttest: %zu wall objects", rt.walls.size());
        float x = (minx + maxx) / 2, z = (minz + maxz) / 2, deg = 0, eye = 100;
        const char *c = getenv("TB_RT_CAM");
        if (c) sscanf(c, "%f,%f,%f,%f", &x, &z, &deg, &eye);
        rt.setCamera(x, eye, z, deg);
        if (const char *w = getenv("TB_RT_WALK")) sscanf(w, "%f,%f", &rt.walk, &rt.spin);
        rt.eye = (int)rt.eyef;
        rt.loadAll();
        rt.activate();
        activate();
    }
};
}  // namespace

void rt_test_register() {
    const char *s = getenv("TB_TEST_RT");
    if (s && *s) static RtTestScene scn(s);
}
