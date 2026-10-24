// The game's WinMain (0x422270), window creation (0x422030), shutdown (0x421f50) and window procedure
// (0x421760), reconstructed. tb_game_main is what platform/core.cpp calls.
#include "engine.h"
#include "tb.h"
#include "../../../platform/ddraw.h"
#include "../../../platform/dinput.h"
#include "../scenes/game.h"
#include <SDL.h>
#include <string.h>

void tb_video_shutdown();   // platform/ddraw.cpp

namespace Vision {

static void *g_hwnd = nullptr;       // 0x5a6674
static bool g_running = false;       // 0x5a7630
bool g_windowed = true;              // 0x5a6678. port: always windowed (the window is letterboxed)
static bool g_dont_abort_close;      // 0x5a68a1

// 0x421760 window procedure
static intptr_t wndproc(void *hwnd, uint32_t msg, uintptr_t wp, intptr_t lp) {
    switch (msg) {
    case TB_WM_SYSCOMMAND: return 0;
    case TB_WM_DESTROY: tb_PostQuitMessage(0); return 0;
    case TB_WM_ACTIVATEAPP:
        set_active(wp != 0);
        stage()->activated();                                   // +0x18
        return 0;
    case TB_WM_CLOSE:                                           // 0x20 in the exe is WM_SETCURSOR; WM_CLOSE goes
        tb_PostQuitMessage(0);                                  // to DefWindowProc -> DestroyWindow -> WM_DESTROY
        return 0;
    case TB_WM_KEYDOWN:
        if (g_developer && wp == TB_VK_ESCAPE) { g_quit = true; return 0; }
        if (is_paused()) return 0;
        key_event((uint32_t)wp, false, true, false, tb_GetKeyState(TB_VK_SHIFT) < 0);
        stage()->input();
        return 0;
    case TB_WM_KEYUP:
        if (is_paused()) return 0;
        key_event((uint32_t)wp, true, false, false, tb_GetKeyState(TB_VK_SHIFT) < 0);
        stage()->input();
        return 0;
    case TB_WM_CHAR:
        if (is_paused()) return 0;
        key_event((uint32_t)wp, false, false, true, tb_GetKeyState(TB_VK_SHIFT) < 0);
        stage()->input();
        return 0;
    case TB_WM_MOUSEMOVE: case TB_WM_LBUTTONDOWN: case TB_WM_LBUTTONUP: case TB_WM_RBUTTONDOWN: case TB_WM_RBUTTONUP: {
        if (is_paused()) return 0;
        // 0x5a7634 / 0x5a7624: half the stage size; the mouse is relative to the centre of the screen
        int cx = stage()->w / 2, cy = stage()->h / 2;
        int x = (int16_t)(lp & 0xffff) - cx, y = (int16_t)((lp >> 16) & 0xffff) - cy;
        bool shift = (wp & TB_MK_SHIFT) != 0, ctrl = (wp & TB_MK_CONTROL) != 0, lb = (wp & TB_MK_LBUTTON) != 0;
        bool up = msg == TB_WM_LBUTTONUP || msg == TB_WM_RBUTTONUP;
        bool down = msg == TB_WM_LBUTTONDOWN || msg == TB_WM_RBUTTONDOWN;
        bool right = msg == TB_WM_RBUTTONDOWN || msg == TB_WM_RBUTTONUP;
        mouse_event(x, y, up, down, msg == TB_WM_MOUSEMOVE, shift, ctrl, lb, right);
        stage()->input();
        return 0;
    }
    }
    return 0;
}

// 0x422030: register the class, create the window, initialise the services, go to the first scene.
static bool create_window() {
    Stage *st = stage();
    g_quit = false;
    g_hwnd = tb_CreateWindowExA(st->name.c_str(), st->name.c_str(), st->w, st->h, wndproc);
    if (!g_hwnd) { vision_log("ABORT: CreateWindow %s failed.", st->name.c_str()); return false; }
    st->beforeServices();                                       // +0x08
    vision_log("Services:");
    for (Node *n = service_kinds().first; n; n = n->next) {
        ServiceKind *k = (ServiceKind *)n->obj;
        vision_log("%s", k->name());
        k->init();                                              // +0x10
        if (g_quit) return false;
    }
    st->afterServices();                                        // +0x10
    tb_ShowWindow(g_hwnd, 1);
    tb_UpdateWindow(g_hwnd);
    g_running = true;
    change_scene(st->first_scene.c_str());
    return true;
}

// 0x421f50
static void shutdown() {
    g_quit = true;
    stage()->aborting();                                        // +0x14
    for (Node *n = service_kinds().last; n; n = n->prev) ((ServiceKind *)n->obj)->shutdown();   // +0x18
    tb_DestroyWindow(g_hwnd);
    stage()->shutdown();                                        // +0x0c
    tb_set_exiting();
    // 0x5a7558/0x5a7548: the original then starts "Launcher.exe" or another program; port: nothing to start.
}

}  // namespace Vision

using namespace Vision;

// 0x422270 WinMain(hInstance, hPrev, lpCmdLine, nShow)
int tb_game_main(int argc, char **argv) {
    tb_game_register();                                         // port: the exe's static constructors (My_Stage etc.)
    Stage *st = stage();
    if (!st || !st->ok()) return 0;                             // +4
    // An argument names the scene to start in (developer mode: Escape quits).
    // port: the exe's own path in the command line is not looked at (it set the working folder).
    if (argc > 1 && argv[1][0] && argv[1][0] != '-') {
        st->first_scene = argv[1];
        g_developer = true;
    }
    // FindWindow(name) + ERROR_ALREADY_EXISTS check (a second copy restores the first one): port: not needed.
    // The CD search (GetLogicalDriveStrings / GetDriveType == 5 / GetVolumeInformation == the stage's cd label,
    // "Please insert the CD-ROM disc into the drive."): port: the game folder is the CD; cd_root() = "".
    cd_root() = "";
    if (!create_window()) { shutdown(); tb_video_shutdown(); return 0; }
    tb_timeGetTime();
    uint32_t last = 0, interval = 0xd05;
    for (;;) {
        uint32_t now = tb_timeGetTime();
        if (now - last > interval) {                            // every ~3.3 s: a new interval (1.5 x)
            last = tb_timeGetTime();
            interval = (uint32_t)(interval * 1.5);              // fild/fmul 0x513b68 (1.5)/ftol (0x4f3948)
        }
        if (g_quit) break;
        if (g_scene_switched) {                                 // drop input queued during the scene change
            g_scene_switched = false;
            TbMsg m;
            while (tb_PeekMessageA(&m, g_hwnd, 0x100, 0x108)) {}
            while (tb_PeekMessageA(&m, g_hwnd, 0x200, 0x209)) {}
        }
        TbMsg m;
        if (tb_PeekMessageA(&m, nullptr, 0, 0)) {
            if (m.message == TB_WM_QUIT) { tb_Sleep(1000 * 0); break; }   // port: the original sleeps 1 s here
            tb_DispatchMessageA(&m);                            // TranslateMessage + DispatchMessage
            continue;
        }
        if (is_active()) st->frame();                           // vtbl +0
        else tb_WaitMessage();
    }
    shutdown();
    tb_video_shutdown();
    return 0;
}
