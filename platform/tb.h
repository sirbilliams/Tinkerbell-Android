// Shared platform core for the Tinker Bell port. Owned by the lead; lanes make minimal additions only.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>

// ---- logging: stderr on desktop, logcat + the shareable log file on Android
void tb_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
const char *tb_log_path();          // the log file the launcher's "Share log" sends (Android)

// ---- where things are
// The installed game folder (the copy of setup/game from the player's ISO): it holds
// "Tinker Bell.exe", data/ and audio/. Desktop: $TB_GAME_DIR or the current directory.
// Android: set by the launcher (see platform/android_*.cpp).
const std::string &tb_game_dir();
void tb_set_game_dir(const std::string &dir);
std::string tb_game_path(const char *rel);   // tb_game_dir() + "/" + rel, '\' turned into '/'
std::string tb_save_dir();                   // writable folder for saves/settings

// ---- headless test runs (desktop, Xvfb)
//   TB_FRAMES=n          quit cleanly after n presented frames
//   TB_SCREENSHOT=x.bmp  save the last presented frame there on exit
//   TB_SHOTS=dir         also save every TB_SHOT_EVERY-th frame (default 30) as dir/NNNNN.bmp
//   TB_WINDOW_SIZE=WxH   window size (default 1280x960; the game is 640x480)
// A clean end of a run prints "CLEAN EXIT".
struct TbHeadless {
    int frames = 0;            // 0 = run until the game quits
    std::string screenshot, shots_dir;
    int shot_every = 30;
};
const TbHeadless &tb_headless();

// ---- values read from the player's own "Tinker Bell.exe" at run time (never copied into the repo)
// Addresses are the exe's virtual addresses (image base 0x400000), as in the decompile.
bool tb_exe_load();                                // maps the exe's sections; false + log if missing
const void *tb_exe_ptr(uint32_t va, size_t len);   // nullptr (and a log line) if not in a plain section
uint32_t tb_exe_u32(uint32_t va);
int32_t  tb_exe_i32(uint32_t va);
const char *tb_exe_str(uint32_t va);               // NUL-terminated string in .rdata/.data

// ---- the game's entry point: the reconstructed WinMain (lane A). platform/main.cpp calls it.
int tb_game_main(int argc, char **argv);
