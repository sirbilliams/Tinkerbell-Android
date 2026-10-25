// Desktop test aid (not in the game): TB_TEST_RT=<scene> renders that 3D scene's walls/ground/sky from a fixed
// camera, TB_RT_CAM="x,z,view,eye" (view = angle index, 0..6*w-1). Start with `tinkerbell rttest`.
#include "raytrace.h"
#include "tb.h"
#include <stdlib.h>
#include <stdio.h>

namespace {
using namespace Vision;

struct TestRT : Raytrace::RayTrace {
    void draw() override {
        horizon = h / 2;
        groundTable();
        castColumns();
        drawSky();
        drawGround2();
    }
};

struct RtTestScene : Scene {
    TestRT rt;
    std::string scene;
    Ras *walls = nullptr, *ground = nullptr;
    explicit RtTestScene(const char *s) : Scene("rttest"), scene(s) {}
    void enter() override {
        walls = ras_open((scene + "_walls").c_str(), "", &data_device());
        walls->load();
        ras_set_current(walls);
        ground = ras_open((scene + "_ground").c_str(), "", &data_device());
        ground->load();
        rt.initAngles();
        rt.range = 4096.0;
        rt.loadWalls(scene.c_str());
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
        rt.camx = (minx + maxx) / 2; rt.camz = (minz + maxz) / 2; rt.view = 0; rt.eyef = 100;
        const char *c = getenv("TB_RT_CAM");
        if (c) sscanf(c, "%f,%f,%d,%f", &rt.camx, &rt.camz, &rt.view, &rt.eyef);
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
