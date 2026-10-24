// DirectDraw-like video shim + the user32/winmm bits the game's WinMain uses (lane A).
// Imitates only what the game uses (see the decompile):
//   0x40dc90 Screen init: DirectDrawCreate, SetCooperativeLevel, SetDisplayMode(640, 480, 16),
//            CreateSurface primary + an offscreen back buffer (DDSCAPS 0x40 | system memory), Lock/Unlock,
//            GetPixelFormat (0x40e0a0: the masks decide the pixel converters).
//   0x41efc0 Screen frame: Lock back buffer, draw, Unlock, Blt back -> primary (vtbl +0x1c).
// Here the "primary" is an SDL window: Blt to it presents the 640x480 RGB565 buffer, letterboxed.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>
#include "dinput.h"

// ---- surfaces (IDirectDrawSurface4)
struct TbSurface {
    int width = 0, height = 0;
    int pitch = 0;                 // bytes
    std::vector<uint16_t> store;
    uint16_t *pixels = nullptr;    // RGB565
    bool primary = false;
    int locks = 0;
    // Lock (vtbl +0x64): pixels / pitch in a DDSURFACEDESC; 0 = DD_OK
    int Lock(uint16_t **out, int *pitch_bytes);
    int Unlock();                  // vtbl +0x80
    // Blt (vtbl +0x1c) of a whole surface: to the primary = present; to another surface = copy at (x, y).
    int Blt(int x, int y, TbSurface *src);
    int IsLost() { return 0; }     // vtbl +0x60
    int Restore() { return 0; }    // vtbl +0x6c
    void Release();                // vtbl +8
};

struct TbPixelFormat { uint32_t rmask, gmask, bmask; int bits; };

struct TbDirectDraw {              // IDirectDraw4
    int SetCooperativeLevel(void *hwnd, uint32_t flags) { return 0; }   // vtbl +0x50
    int SetDisplayMode(int w, int h, int bpp);                          // vtbl +0x54
    // vtbl +0x18. caps: 1 = primary, else an offscreen w x h surface.
    int CreateSurface(bool primary, int w, int h, TbSurface **out);
    int GetDisplayBpp() { return 16; }                                  // EnumDisplayModes (vtbl +0x20) result
    void Release();
};
int tb_DirectDrawCreate(TbDirectDraw **out);
void tb_GetPixelFormat(TbSurface *s, TbPixelFormat *pf);              // vtbl +0x54: RGB565

// Present an arbitrary RGB565 frame (QuickTime drew movies straight to the window; the port presents them here).
void tb_present_565(const uint16_t *pix, int w, int h, int pitch_bytes);
int tb_presented_frames();
// The headless frame budget (TB_FRAMES) ran out: the window gets WM_CLOSE.
bool tb_frames_done();

// ---- user32 / winmm (the WinMain and the message loop)
enum { TB_WM_DESTROY = 2, TB_WM_CLOSE = 0x10, TB_WM_QUIT = 0x12, TB_WM_ACTIVATEAPP = 0x1c, TB_WM_SYSCOMMAND = 0x112 };
typedef intptr_t (*TbWndProc)(void *hwnd, uint32_t msg, uintptr_t wp, intptr_t lp);
void *tb_CreateWindowExA(const char *cls, const char *title, int w, int h, TbWndProc proc);
void tb_ShowWindow(void *hwnd, int cmd);
void tb_UpdateWindow(void *hwnd);
void tb_DestroyWindow(void *hwnd);
void tb_PostQuitMessage(int code);
// PeekMessage(PM_REMOVE) for any window; filter (first, last) as Win32 (0, 0 = all).
bool tb_PeekMessageA(TbMsg *m, void *hwnd, uint32_t first, uint32_t last);
void tb_DispatchMessageA(const TbMsg *m);
void tb_WaitMessage();
uint32_t tb_timeGetTime();
void tb_Sleep(uint32_t ms);
int tb_MessageBoxA(void *hwnd, const char *text, const char *caption, uint32_t type);
