// 0x425d40, the game's random number: (rand() * n) >> 15 with the C runtime's rand(). rand() lives in the
// encrypted runtime pages; this is MSVC's documented generator (holdrand * 214013 + 2531011, bits 16..30).
// port: seeded once from the clock at the first call (the exe's srand call, if any, is in the runtime pages).
#include "engine.h"
#include <time.h>

namespace Vision {
static uint32_t g_holdrand = 0;
static bool g_seeded = false;
static int msvc_rand() {
    if (!g_seeded) { g_holdrand = (uint32_t)time(nullptr); g_seeded = true; }
    g_holdrand = g_holdrand * 214013u + 2531011u;
    return (int)((g_holdrand >> 16) & 0x7fff);
}
int vision_random(int n) { return (int)((uint32_t)(msvc_rand() * n) >> 15); }
}  // namespace Vision
