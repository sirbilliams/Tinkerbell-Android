// Logging, paths, headless settings and the process entry point.
#include "tb.h"
#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace {
std::string g_game_dir, g_log_path;
FILE *g_logf = nullptr;
TbHeadless g_headless;
}

void tb_log(const char *fmt, ...) {
    char buf[2048];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "TinkerBell", "%s", buf);
#else
    fprintf(stderr, "%s\n", buf);
#endif
    if (g_logf) { fprintf(g_logf, "%s\n", buf); fflush(g_logf); }
}

const char *tb_log_path() { return g_log_path.c_str(); }

const std::string &tb_game_dir() { return g_game_dir; }
void tb_set_game_dir(const std::string &dir) { g_game_dir = dir; }

std::string tb_game_path(const char *rel) {
    std::string p = g_game_dir.empty() ? std::string(".") : g_game_dir;
    p += '/';
    for (const char *c = rel; *c; c++) p += *c == '\\' ? '/' : *c;
    return p;
}

std::string tb_save_dir() {
    std::string d = tb_game_path("save");
    mkdir(d.c_str(), 0755);
    return d;
}

const TbHeadless &tb_headless() { return g_headless; }

// Weak default until the reconstructed WinMain lands (lane A replaces it).
__attribute__((weak)) int tb_game_main(int, char **) {
    tb_log("tb_game_main: not ported yet");
    return 0;
}

// Lane B's desktop test entry (platform/media_test.cpp): returns -1 unless TB_TEST_SOUND / TB_TEST_MOVIE is set.
int tb_test_hook(int, char **) __attribute__((weak));

extern "C" int main(int argc, char **argv) {
    const char *e;
    if ((e = getenv("TB_GAME_DIR")) && *e) g_game_dir = e;
    if ((e = getenv("TB_FRAMES"))) g_headless.frames = atoi(e);
    if ((e = getenv("TB_SCREENSHOT"))) g_headless.screenshot = e;
    if ((e = getenv("TB_SHOTS"))) g_headless.shots_dir = e;
    if ((e = getenv("TB_SHOT_EVERY")) && atoi(e) > 0) g_headless.shot_every = atoi(e);
#ifdef __ANDROID__
    // The launcher passes the install folder; the log goes next to it so "Share log" can send it.
    if ((e = getenv("TB_ANDROID_GAME_DIR")) && *e) g_game_dir = e;
#endif
    g_log_path = tb_game_path("tinkerbell.log");
    g_logf = fopen(g_log_path.c_str(), "w");
    tb_log("Tinker Bell port: game dir %s", g_game_dir.empty() ? "." : g_game_dir.c_str());
    if (!tb_exe_load()) return 1;
    int rc = tb_test_hook ? tb_test_hook(argc, argv) : -1;
    if (rc < 0) rc = tb_game_main(argc, argv);
    tb_log("CLEAN EXIT (%d)", rc);
    if (g_logf) fclose(g_logf);
    return rc;
}
