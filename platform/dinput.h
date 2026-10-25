// Input shim (lane C). The game reads input from Windows messages in its window procedure (0x421760:
// WM_KEYDOWN/KEYUP/CHAR, WM_MOUSEMOVE, WM_L/R/MBUTTONDOWN/UP, GetKeyState for Shift/Ctrl); its DirectInput
// service (0x4182a0) only creates the IDirectInput8 object and never makes a device.
// This turns SDL events into those messages:
//   * mouse (desktop), touch on the top screen (Android: SDL's touch->mouse: tap = click, drag = held button),
//   * gamepad: sticks / d-pad move the cursor, A = left click, B/X = right click, Start / Back = Escape.
//
// How the window loop (lane A) uses it:
//     tb_input_init();                                  // after SDL_Init
//     tb_input_set_view(dst_x, dst_y, dst_w, dst_h, 640, 480);   // the letterboxed game rectangle in the window
//     each frame:  while (SDL_PollEvent(&e)) { if (!tb_input_event(e)) /* quit, resize, ... yours */; }
//                  tb_input_update();                   // gamepad cursor motion
//                  TbMsg m; while (tb_input_pop(&m)) WndProc(hwnd, m.message, m.wParam, m.lParam);
//     GetKeyState(vk) -> tb_GetKeyState(vk); GetCursorPos -> tb_GetCursorPos (game coordinates)
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <SDL.h>

enum {
    TB_WM_KEYDOWN = 0x100, TB_WM_KEYUP = 0x101, TB_WM_CHAR = 0x102,
    TB_WM_MOUSEMOVE = 0x200, TB_WM_LBUTTONDOWN = 0x201, TB_WM_LBUTTONUP = 0x202,
    TB_WM_RBUTTONDOWN = 0x204, TB_WM_RBUTTONUP = 0x205, TB_WM_MBUTTONDOWN = 0x207, TB_WM_MBUTTONUP = 0x208,
    TB_MK_LBUTTON = 1, TB_MK_RBUTTON = 2, TB_MK_SHIFT = 4, TB_MK_CONTROL = 8, TB_MK_MBUTTON = 0x10,
    TB_VK_LBUTTON = 1, TB_VK_RBUTTON = 2, TB_VK_MBUTTON = 4, TB_VK_BACK = 8, TB_VK_TAB = 9, TB_VK_RETURN = 13,
    TB_VK_SHIFT = 0x10, TB_VK_CONTROL = 0x11, TB_VK_MENU = 0x12, TB_VK_ESCAPE = 0x1B, TB_VK_SPACE = 0x20,
};

// A window message as the window procedure gets it. Mouse messages: lParam = MAKELONG(x, y) in game pixels
// (client coordinates of the 640x480 game window), wParam = MK_ flags. Key messages: wParam = virtual key,
// lParam = repeat count | scan code << 16 | bit 30 (was down) | bit 31 (up). WM_CHAR: wParam = Windows ANSI char.
struct TbMsg {
    uint32_t message;
    uintptr_t wParam;
    intptr_t lParam;
    uint32_t time;     // ms
};

void tb_input_init();                 // hints, game controllers; safe to call twice
void tb_input_shutdown();
// The game rectangle inside the window (pixels) and the game's own size; maps window -> game coordinates.
void tb_input_set_view(int dst_x, int dst_y, int dst_w, int dst_h, int game_w, int game_h);
bool tb_input_event(const SDL_Event &e);   // true if it was an input event (consumed)
void tb_input_update();                    // call once per frame: gamepad motion
bool tb_input_pop(TbMsg *m);               // next queued message (PeekMessage + PM_REMOVE); false if none
void tb_input_clear();                     // drop everything queued and release all keys (lost focus)
// port: in the 3D scenes the d-pad / left stick act as the arrow keys and the shoulders as Shift (up) / Ctrl
// (down), the right stick moves the cursor; the scenes turn this on and off.
void tb_input_set_arrows(bool on);
int16_t tb_GetKeyState(int vk);            // user32 GetKeyState: negative when down, bit 0 = toggled (CapsLock)
void tb_GetCursorPos(int *x, int *y);      // the cursor, in game coordinates

// ---- DirectInput8 (the game calls DirectInput8Create and Releases it; nothing else)
struct TbDirectInput8 { int refs; };
// 0x4f1f4a DirectInput8Create(hInstance, 0x800, IID_IDirectInput8A, &ppv, NULL): always succeeds (returns 0 = DI_OK).
int32_t tb_DirectInput8Create(void *hinst, uint32_t version, const void *iid, TbDirectInput8 **out, void *outer);
void tb_DirectInput8_Release(TbDirectInput8 *di);
