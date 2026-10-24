// DirectDraw-like video shim and the user32/winmm calls of the game's WinMain, on SDL2 (lane A).
#include "ddraw.h"
#include "tb.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <deque>

namespace {
SDL_Window *g_win = nullptr;
SDL_Renderer *g_ren = nullptr;
SDL_Texture *g_tex = nullptr;
TbWndProc g_proc = nullptr;
std::deque<TbMsg> g_posted;         // messages posted by the game itself (WM_QUIT, WM_CLOSE)
int g_presented = 0;
bool g_frames_done = false;
std::vector<uint16_t> g_last;       // last presented frame (screenshot)
int g_last_w = 0, g_last_h = 0;
int g_hwnd_token;                   // the HWND is the address of this

bool save_bmp(const char *path, const uint16_t *pix, int w, int h) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom((void *)pix, w, h, 16, w * 2, SDL_PIXELFORMAT_RGB565);
    if (!s) return false;
    bool ok = SDL_SaveBMP(s, path) == 0;
    SDL_FreeSurface(s);
    return ok;
}

void make_window(int w, int h, const char *title) {
    if (g_win) return;
    int ww = 1280, wh = 960;
    const char *e = getenv("TB_WINDOW_SIZE");
    if (e) sscanf(e, "%dx%d", &ww, &wh);
    g_win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh,
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!g_win) { tb_log("SDL_CreateWindow: %s", SDL_GetError()); return; }
    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    tb_input_init();
}

void letterbox(int gw, int gh, SDL_Rect *dst) {
    int ow = 0, oh = 0;
    SDL_GetRendererOutputSize(g_ren, &ow, &oh);
    int ww = 0, wh = 0;
    SDL_GetWindowSize(g_win, &ww, &wh);
    if (ow <= 0 || oh <= 0) { ow = gw; oh = gh; }
    if ((int64_t)ow * gh > (int64_t)oh * gw) { dst->h = oh; dst->w = oh * gw / gh; }
    else { dst->w = ow; dst->h = ow * gh / gw; }
    dst->x = (ow - dst->w) / 2;
    dst->y = (oh - dst->h) / 2;
    // the input shim wants window coordinates
    double sx = ww > 0 ? (double)ww / ow : 1.0, sy = wh > 0 ? (double)wh / oh : 1.0;
    tb_input_set_view((int)(dst->x * sx), (int)(dst->y * sy), (int)(dst->w * sx), (int)(dst->h * sy), gw, gh);
}
}  // namespace

// ---- surfaces
int TbSurface::Lock(uint16_t **out, int *pitch_bytes) {
    locks++;
    *out = pixels;
    *pitch_bytes = pitch;
    return 0;
}
int TbSurface::Unlock() { if (locks > 0) locks--; return 0; }

int TbSurface::Blt(int x, int y, TbSurface *src) {
    if (!src || !src->pixels) return 0;
    if (primary) {                                   // port: the primary surface is the SDL window
        tb_present_565(src->pixels, src->width, src->height, src->pitch);
        return 0;
    }
    for (int r = 0; r < src->height; r++) {
        int dy = y + r;
        if (dy < 0 || dy >= height) continue;
        for (int c = 0; c < src->width; c++) {
            int dx = x + c;
            if (dx < 0 || dx >= width) continue;
            pixels[dy * (pitch / 2) + dx] = src->pixels[r * (src->pitch / 2) + c];
        }
    }
    return 0;
}

void TbSurface::Release() { delete this; }

int TbDirectDraw::SetDisplayMode(int w, int h, int bpp) { return 0; }   // port: windowed, the window is letterboxed

int TbDirectDraw::CreateSurface(bool primary, int w, int h, TbSurface **out) {
    TbSurface *s = new TbSurface;
    s->primary = primary;
    if (primary) { w = 640; h = 480; }
    s->width = w; s->height = h; s->pitch = w * 2;
    s->store.assign((size_t)w * h, 0);
    s->pixels = s->store.data();
    *out = s;
    return 0;
}

void TbDirectDraw::Release() { delete this; }

int tb_DirectDrawCreate(TbDirectDraw **out) { *out = new TbDirectDraw; return 0; }

void tb_GetPixelFormat(TbSurface *, TbPixelFormat *pf) { pf->rmask = 0xf800; pf->gmask = 0x7e0; pf->bmask = 0x1f; pf->bits = 16; }

// ---- present
void tb_present_565(const uint16_t *pix, int w, int h, int pitch_bytes) {
    const TbHeadless &hl = tb_headless();
    g_last.resize((size_t)w * h);
    for (int r = 0; r < h; r++) memcpy(&g_last[(size_t)r * w], (const uint8_t *)pix + (size_t)r * pitch_bytes, w * 2);
    g_last_w = w; g_last_h = h;
    if (g_ren) {
        if (!g_tex) g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (g_tex) {
            SDL_UpdateTexture(g_tex, nullptr, g_last.data(), w * 2);
            SDL_Rect dst;
            letterbox(w, h, &dst);
            SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
            SDL_RenderClear(g_ren);
            SDL_RenderCopy(g_ren, g_tex, nullptr, &dst);
            SDL_RenderPresent(g_ren);
        }
    }
    g_presented++;
    if (!hl.shots_dir.empty() && g_presented % hl.shot_every == 0) {
        char p[1024];
        snprintf(p, sizeof p, "%s/%05d.bmp", hl.shots_dir.c_str(), g_presented);
        save_bmp(p, g_last.data(), w, h);
    }
    if (hl.frames > 0 && g_presented >= hl.frames && !g_frames_done) {
        g_frames_done = true;
        TbMsg m = {TB_WM_CLOSE, 0, 0, SDL_GetTicks()};   // port: headless runs end like a closed window
        g_posted.push_back(m);
    }
}

int tb_presented_frames() { return g_presented; }
bool tb_frames_done() { return g_frames_done; }

// Called at the end of tb_game_main: the screenshot of the last frame.
void tb_video_shutdown() {
    const TbHeadless &hl = tb_headless();
    if (!hl.screenshot.empty()) {
        if (g_last.empty()) tb_log("screenshot: nothing was presented");
        else if (save_bmp(hl.screenshot.c_str(), g_last.data(), g_last_w, g_last_h))
            tb_log("screenshot: %s (frame %d)", hl.screenshot.c_str(), g_presented);
    }
    if (g_tex) SDL_DestroyTexture(g_tex);
    if (g_ren) SDL_DestroyRenderer(g_ren);
    if (g_win) SDL_DestroyWindow(g_win);
    g_tex = nullptr; g_ren = nullptr; g_win = nullptr;
}

// ---- user32
void *tb_CreateWindowExA(const char *cls, const char *title, int w, int h, TbWndProc proc) {
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0 && SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        tb_log("SDL video: %s", SDL_GetError());
        return nullptr;
    }
    g_proc = proc;
    make_window(w, h, title);
    return g_win ? &g_hwnd_token : nullptr;
}
void tb_ShowWindow(void *, int) { if (g_win) SDL_ShowWindow(g_win); }
void tb_UpdateWindow(void *) {}
void tb_DestroyWindow(void *hwnd) {
    TbMsg m = {TB_WM_DESTROY, 0, 0, SDL_GetTicks()};
    if (g_proc) g_proc(hwnd, m.message, 0, 0);
}
void tb_PostQuitMessage(int code) { TbMsg m = {TB_WM_QUIT, (uintptr_t)code, 0, SDL_GetTicks()}; g_posted.push_back(m); }

static void pump() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (tb_input_event(e)) continue;
        if (e.type == SDL_QUIT) { TbMsg m = {TB_WM_CLOSE, 0, 0, SDL_GetTicks()}; g_posted.push_back(m); }
        else if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                tb_input_clear();
                TbMsg m = {TB_WM_ACTIVATEAPP, 0, 0, SDL_GetTicks()}; g_posted.push_back(m);
            } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                TbMsg m = {TB_WM_ACTIVATEAPP, 1, 0, SDL_GetTicks()}; g_posted.push_back(m);
            }
        }
    }
    tb_input_update();
}

bool tb_PeekMessageA(TbMsg *m, void *, uint32_t first, uint32_t last) {
    pump();
    for (size_t i = 0; i < g_posted.size(); i++) {
        if (first || last) { if (g_posted[i].message < first || g_posted[i].message > last) continue; }
        *m = g_posted[i];
        g_posted.erase(g_posted.begin() + i);
        return true;
    }
    // input messages (lane C's queue); with a filter they are taken only if they match
    TbMsg q;
    if (first || last) {
        // port: the game only filters to drop queued input (0x100-0x108, 0x200-0x209); drop the whole queue then
        if ((first == 0x100 || first == 0x200) && tb_input_pop(&q)) { *m = q; return true; }
        return false;
    }
    if (tb_input_pop(&q)) { *m = q; return true; }
    return false;
}

void tb_DispatchMessageA(const TbMsg *m) { if (g_proc) g_proc(&g_hwnd_token, m->message, m->wParam, m->lParam); }
void tb_WaitMessage() { SDL_WaitEventTimeout(nullptr, 10); }
uint32_t tb_timeGetTime() { return SDL_GetTicks(); }
void tb_Sleep(uint32_t ms) { SDL_Delay(ms); }
int tb_MessageBoxA(void *, const char *text, const char *caption, uint32_t) {
    tb_log("MessageBox [%s]: %s", caption ? caption : "", text ? text : "");
    return 1;   // IDOK
}
