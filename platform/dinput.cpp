// Input shim: SDL events -> Win32 messages (see dinput.h).
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dinput.h"
#include "tb.h"
#include <deque>
#include <math.h>
#include <string.h>

namespace {

std::deque<TbMsg> g_q;
bool g_down[256];                 // by virtual key
bool g_caps = false;
int g_dx = 0, g_dy = 0, g_dw = 640, g_dh = 480, g_gw = 640, g_gh = 480;
float g_cx = 320, g_cy = 240;     // the cursor, game coordinates
SDL_GameController *g_pads[8];
bool g_inited = false;
uint32_t g_last_ticks = 0;
float g_ax = 0, g_ay = 0;         // stick / d-pad direction, -1..1
bool g_dpad[4];                   // up down left right
bool g_arrows = false;            // 3D scenes: d-pad / left stick = arrow keys, shoulders = Shift / Ctrl
bool g_shoulder[2];               // left, right
bool g_pad_btn[4];                // A, B, X, Start

uint32_t now() { return SDL_GetTicks(); }

void post(uint32_t msg, uintptr_t w, intptr_t l) { g_q.push_back({ msg, w, l, now() }); }

uintptr_t mouseFlags() {
    uintptr_t f = 0;
    if (g_down[TB_VK_LBUTTON]) f |= TB_MK_LBUTTON;
    if (g_down[TB_VK_RBUTTON]) f |= TB_MK_RBUTTON;
    if (g_down[TB_VK_MBUTTON]) f |= TB_MK_MBUTTON;
    if (g_down[TB_VK_SHIFT]) f |= TB_MK_SHIFT;
    if (g_down[TB_VK_CONTROL]) f |= TB_MK_CONTROL;
    return f;
}

int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
intptr_t xy() { return (intptr_t)(((uint32_t)clampi((int)g_cy, 0, g_gh - 1) << 16) | (uint32_t)clampi((int)g_cx, 0, g_gw - 1)); }

intptr_t g_last_xy = -1;
void postMove() {
    intptr_t p = xy();
    if (p == g_last_xy) return;      // WM_MOUSEMOVE only when the pixel changed
    g_last_xy = p;
    post(TB_WM_MOUSEMOVE, mouseFlags(), p);
}

void setCursorFromWindow(int wx, int wy) {
    g_cx = (float)((wx - g_dx) * g_gw) / (g_dw > 0 ? g_dw : 1);
    g_cy = (float)((wy - g_dy) * g_gh) / (g_dh > 0 ? g_dh : 1);
    g_cx = (float)clampi((int)floorf(g_cx), 0, g_gw - 1);
    g_cy = (float)clampi((int)floorf(g_cy), 0, g_gh - 1);
}

bool insideView(int wx, int wy) { return wx >= g_dx && wy >= g_dy && wx < g_dx + g_dw && wy < g_dy + g_dh; }

void button(int vk, bool down) {
    if (g_down[vk] == down) return;
    g_down[vk] = down;
    uint32_t msg;
    switch (vk) {
    case TB_VK_LBUTTON: msg = down ? TB_WM_LBUTTONDOWN : TB_WM_LBUTTONUP; break;
    case TB_VK_RBUTTON: msg = down ? TB_WM_RBUTTONDOWN : TB_WM_RBUTTONUP; break;
    default: msg = down ? TB_WM_MBUTTONDOWN : TB_WM_MBUTTONUP; break;
    }
    post(msg, mouseFlags(), xy());    // as Windows: a down includes its own button, an up no longer does
}

// SDL scancode -> Windows virtual key (0 if none)
int vkFromScancode(SDL_Scancode sc) {
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) return 'A' + (sc - SDL_SCANCODE_A);
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9) return '1' + (sc - SDL_SCANCODE_1);
    if (sc == SDL_SCANCODE_0) return '0';
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12) return 0x70 + (sc - SDL_SCANCODE_F1);
    if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_9) return 0x61 + (sc - SDL_SCANCODE_KP_1);
    switch (sc) {
    case SDL_SCANCODE_KP_0: return 0x60;
    case SDL_SCANCODE_KP_MULTIPLY: return 0x6A; case SDL_SCANCODE_KP_PLUS: return 0x6B;
    case SDL_SCANCODE_KP_MINUS: return 0x6D; case SDL_SCANCODE_KP_PERIOD: return 0x6E; case SDL_SCANCODE_KP_DIVIDE: return 0x6F;
    case SDL_SCANCODE_KP_ENTER: case SDL_SCANCODE_RETURN: return TB_VK_RETURN;
    case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_AC_BACK: return TB_VK_ESCAPE;
    case SDL_SCANCODE_BACKSPACE: return TB_VK_BACK; case SDL_SCANCODE_TAB: return TB_VK_TAB;
    case SDL_SCANCODE_SPACE: return TB_VK_SPACE;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return TB_VK_SHIFT;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return TB_VK_CONTROL;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return TB_VK_MENU;
    case SDL_SCANCODE_CAPSLOCK: return 0x14; case SDL_SCANCODE_PAUSE: return 0x13;
    case SDL_SCANCODE_PAGEUP: return 0x21; case SDL_SCANCODE_PAGEDOWN: return 0x22;
    case SDL_SCANCODE_END: return 0x23; case SDL_SCANCODE_HOME: return 0x24;
    case SDL_SCANCODE_LEFT: return 0x25; case SDL_SCANCODE_UP: return 0x26;
    case SDL_SCANCODE_RIGHT: return 0x27; case SDL_SCANCODE_DOWN: return 0x28;
    case SDL_SCANCODE_INSERT: return 0x2D; case SDL_SCANCODE_DELETE: return 0x2E;
    case SDL_SCANCODE_SEMICOLON: return 0xBA; case SDL_SCANCODE_EQUALS: return 0xBB; case SDL_SCANCODE_COMMA: return 0xBC;
    case SDL_SCANCODE_MINUS: return 0xBD; case SDL_SCANCODE_PERIOD: return 0xBE; case SDL_SCANCODE_SLASH: return 0xBF;
    case SDL_SCANCODE_GRAVE: return 0xC0; case SDL_SCANCODE_LEFTBRACKET: return 0xDB; case SDL_SCANCODE_BACKSLASH: return 0xDC;
    case SDL_SCANCODE_RIGHTBRACKET: return 0xDD; case SDL_SCANCODE_APOSTROPHE: return 0xDE;
    default: return 0;
    }
}

void key(int vk, int scancode, bool down, bool repeat) {
    if (!vk) return;
    bool was = g_down[vk];
    // Shift / Ctrl / Alt appear once for both sides: only report changes of the combined state
    g_down[vk] = down;
    if (vk == 0x14 && down && !repeat) g_caps = !g_caps;
    uint32_t u = 1 | ((uint32_t)(scancode & 0xFF) << 16);
    if (down) { if (was || repeat) u |= 1u << 30; }
    else u |= (1u << 30) | 0x80000000u;
    intptr_t l = (intptr_t)(int32_t)u;      // a 32-bit LPARAM
    post(down ? TB_WM_KEYDOWN : TB_WM_KEYUP, (uintptr_t)vk, l);
    // TranslateMessage: the keys that make a WM_CHAR without text input
    if (down) {
        if (vk == TB_VK_BACK) post(TB_WM_CHAR, 8, l);
        else if (vk == TB_VK_TAB) post(TB_WM_CHAR, 9, l);
        else if (vk == TB_VK_RETURN) post(TB_WM_CHAR, 13, l);
        else if (vk == TB_VK_ESCAPE) post(TB_WM_CHAR, 27, l);
    }
}

// UTF-8 text from SDL -> Windows ANSI (CP1252) WM_CHARs
void text(const char *u) {
    const unsigned char *p = (const unsigned char *)u;
    while (*p) {
        uint32_t cp;
        if (*p < 0x80) cp = *p++;
        else if ((*p & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) { cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
        else if ((*p & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) { cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; }
        else { p++; continue; }
        static const uint16_t cp1252[32] = {
            0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x8D, 0x017D, 0x8F,
            0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D, 0x017E, 0x0178 };
        int ch = -1;
        if (cp >= 32 && cp < 0x7F) ch = (int)cp;
        else if (cp >= 0xA0 && cp <= 0xFF) ch = (int)cp;
        else for (int i = 0; i < 32; i++) if (cp1252[i] == cp) { ch = 0x80 + i; break; }
        if (ch >= 32) post(TB_WM_CHAR, (uintptr_t)ch, 1);
    }
}

void openPad(int index) {
    if (!SDL_IsGameController(index)) return;
    for (auto &p : g_pads) if (p && SDL_GameControllerGetAttached(p) && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p)) == SDL_JoystickGetDeviceInstanceID(index)) return;
    for (auto &p : g_pads) if (!p) {
        p = SDL_GameControllerOpen(index);
        if (p) tb_log("input: gamepad \"%s\"", SDL_GameControllerName(p));
        return;
    }
}

void closePad(SDL_JoystickID id) {
    for (auto &p : g_pads)
        if (p && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p)) == id) { SDL_GameControllerClose(p); p = nullptr; }
}

void padButton(int which, bool down) {
    if (g_pad_btn[which] == down) return;
    g_pad_btn[which] = down;
    switch (which) {
    case 0: button(TB_VK_LBUTTON, down); break;                 // A: left click
    case 1: case 2: button(TB_VK_RBUTTON, down); break;         // B / X: right click
    case 3: key(TB_VK_ESCAPE, 0x01, down, false); break;        // Start: Escape
    }
}

float axisValue(SDL_GameController *p, SDL_GameControllerAxis a) {
    int v = SDL_GameControllerGetAxis(p, a);
    const int dead = 7500;
    if (v > -dead && v < dead) return 0;
    float f = (float)(v > 0 ? v - dead : v + dead) / (32767.0f - dead);
    return f > 1 ? 1 : f < -1 ? -1 : f;
}

}  // namespace

void tb_input_init() {
    if (g_inited) return;
    g_inited = true;
    SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER);
    // touch becomes the mouse (a tap is a click, a drag holds the left button); a real mouse is untouched
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    for (int i = 0; i < SDL_NumJoysticks(); i++) openPad(i);
    g_last_ticks = now();
    memset(g_down, 0, sizeof g_down);
}

void tb_input_shutdown() {
    for (auto &p : g_pads) if (p) { SDL_GameControllerClose(p); p = nullptr; }
    g_q.clear();
    g_inited = false;
}

void tb_input_set_view(int dx, int dy, int dw, int dh, int gw, int gh) {
    g_dx = dx; g_dy = dy; g_dw = dw; g_dh = dh; g_gw = gw > 0 ? gw : 640; g_gh = gh > 0 ? gh : 480;
}

bool tb_input_event(const SDL_Event &e) {
    switch (e.type) {
    case SDL_MOUSEMOTION:
        setCursorFromWindow(e.motion.x, e.motion.y);
        postMove();
        return true;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        bool down = e.type == SDL_MOUSEBUTTONDOWN;
        if (down && !insideView(e.button.x, e.button.y)) return true;   // a tap on the black bars is not a click
        setCursorFromWindow(e.button.x, e.button.y);
        int vk = e.button.button == SDL_BUTTON_LEFT ? TB_VK_LBUTTON : e.button.button == SDL_BUTTON_RIGHT ? TB_VK_RBUTTON
               : e.button.button == SDL_BUTTON_MIDDLE ? TB_VK_MBUTTON : 0;
        if (!vk) return true;
        if (down != g_down[vk]) postMove();     // the position lands before the click (a tap jumps, then presses)
        button(vk, down);
        return true;
    }
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        key(vkFromScancode(e.key.keysym.scancode), e.key.keysym.scancode & 0xFF, e.type == SDL_KEYDOWN, e.key.repeat != 0);
        return true;
    case SDL_TEXTINPUT:
        text(e.text.text);
        return true;
    case SDL_CONTROLLERDEVICEADDED: openPad(e.cdevice.which); return true;
    case SDL_CONTROLLERDEVICEREMOVED: closePad(e.cdevice.which); return true;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP: {
        bool down = e.type == SDL_CONTROLLERBUTTONDOWN;
        switch (e.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A: padButton(0, down); break;
        case SDL_CONTROLLER_BUTTON_B: padButton(1, down); break;
        case SDL_CONTROLLER_BUTTON_X: padButton(2, down); break;
        case SDL_CONTROLLER_BUTTON_START: case SDL_CONTROLLER_BUTTON_BACK: padButton(3, down); break;
        // in the 3D scenes the shoulders are also Ctrl / Shift and Y is Enter as key events (the hover keys)
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            if (g_shoulder[0] != down && g_arrows) key(0x11, 0x1d, down, false);
            g_shoulder[0] = down;
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            if (g_shoulder[1] != down && g_arrows) key(0x10, 0x2a, down, false);
            g_shoulder[1] = down;
            break;
        case SDL_CONTROLLER_BUTTON_Y: if (g_arrows) key(0x0d, 0x1c, down, false); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: g_dpad[0] = down; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: g_dpad[1] = down; break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: g_dpad[2] = down; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: g_dpad[3] = down; break;
        default: break;
        }
        return true;
    }
    case SDL_CONTROLLERAXISMOTION:
        return true;       // read in tb_input_update
    default:
        return false;
    }
}

// Desktop test aid (not in the game): TB_FAKE_INPUT="<frame>:click <x>,<y>;<frame>:move <x>,<y>;<frame>:key <vk>"
// replays mouse clicks / moves (game pixels) and key presses at the given frame numbers (calls of tb_input_update).
static int g_input_frame = 0;
int tb_input_frame() { return g_input_frame; }      // test aid: the frame number TB_FAKE_INPUT counts
static void fakeInput() {
    static const char *spec = getenv("TB_FAKE_INPUT");
    int frame = ++g_input_frame;
    if (!spec || !*spec) return;
    for (const char *p = spec; p && *p; ) {
        int f = 0, a = 0, b = 0, n = 0;
        char what[8] = {0};
        if (sscanf(p, "%d:%7s %d,%d%n", &f, what, &a, &b, &n) >= 3 && f == frame) {
            if (!strcmp(what, "click") || !strcmp(what, "move")) {
                g_cx = (float)a; g_cy = (float)b;
                postMove();
                if (!strcmp(what, "click")) { button(TB_VK_LBUTTON, true); button(TB_VK_LBUTTON, false); }
            } else if (!strcmp(what, "key")) {
                key(a, 0, true, false); key(a, 0, false, false);
            } else if (!strcmp(what, "down")) {     // "down vk,0": held until "up vk,0"
                key(a, 0, true, false);
            } else if (!strcmp(what, "up")) {
                key(a, 0, false, false);
            }
        }
        p = strchr(p, ';');
        if (p) p++;
    }
}

void tb_input_update() {
    fakeInput();
    uint32_t t = now();
    float dt = (t - g_last_ticks) / 1000.0f;
    g_last_ticks = t;
    if (dt > 0.1f) dt = 0.1f;
    float ax = 0, ay = 0;
    for (auto p : g_pads) {
        if (!p) continue;
        if (!g_arrows) {
            ax += axisValue(p, SDL_CONTROLLER_AXIS_LEFTX);
            ay += axisValue(p, SDL_CONTROLLER_AXIS_LEFTY);
        }
        ax += axisValue(p, SDL_CONTROLLER_AXIS_RIGHTX);
        ay += axisValue(p, SDL_CONTROLLER_AXIS_RIGHTY);
    }
    float sx = ax * fabsf(ax), sy = ay * fabsf(ay);                // finer control near the centre
    float speed = 420.0f;                                           // game pixels per second at full tilt
    float mx = sx * speed, my = sy * speed;
    if (!g_arrows) {
        if (g_dpad[2]) mx -= 240;
        if (g_dpad[3]) mx += 240;
        if (g_dpad[0]) my -= 240;
        if (g_dpad[1]) my += 240;
    }
    if (mx != 0 || my != 0) {
        float nx = g_cx + mx * dt, ny = g_cy + my * dt;
        nx = nx < 0 ? 0 : nx > g_gw - 1 ? (float)(g_gw - 1) : nx;
        ny = ny < 0 ? 0 : ny > g_gh - 1 ? (float)(g_gh - 1) : ny;
        bool moved = (int)nx != (int)g_cx || (int)ny != (int)g_cy;
        g_cx = nx; g_cy = ny;      // keep the fraction: slow motion still accumulates
        if (moved) postMove();
    }
}

bool tb_input_pop(TbMsg *m) {
    if (g_q.empty()) return false;
    *m = g_q.front();
    g_q.pop_front();
    return true;
}

void tb_input_clear() {
    g_q.clear();
    memset(g_down, 0, sizeof g_down);
    memset(g_pad_btn, 0, sizeof g_pad_btn);
    memset(g_dpad, 0, sizeof g_dpad);
    memset(g_shoulder, 0, sizeof g_shoulder);
}

void tb_input_set_arrows(bool on) { g_arrows = on; }

// The pad as keys in 3D scenes: is `vk` held through the d-pad, left stick or shoulders?
static bool padKey(int vk) {
    if (!g_arrows) return false;
    float lx = 0, ly = 0;
    for (auto p : g_pads) {
        if (!p) continue;
        lx += axisValue(p, SDL_CONTROLLER_AXIS_LEFTX);
        ly += axisValue(p, SDL_CONTROLLER_AXIS_LEFTY);
    }
    switch (vk) {
    case 0x26: return g_dpad[0] || ly < -0.5f;      // VK_UP
    case 0x28: return g_dpad[1] || ly > 0.5f;       // VK_DOWN
    case 0x25: return g_dpad[2] || lx < -0.5f;      // VK_LEFT
    case 0x27: return g_dpad[3] || lx > 0.5f;       // VK_RIGHT
    case 0x10: return g_shoulder[1];                // VK_SHIFT: fly up
    case 0x11: return g_shoulder[0];                // VK_CONTROL: fly down
    }
    return false;
}

int16_t tb_GetKeyState(int vk) {
    if (vk < 0 || vk > 255) return 0;
    int16_t r = (g_down[vk] || padKey(vk)) ? (int16_t)0x8000 : 0;
    if (vk == 0x14 && g_caps) r |= 1;
    return r;
}

void tb_GetCursorPos(int *x, int *y) {
    if (x) *x = clampi((int)g_cx, 0, g_gw - 1);
    if (y) *y = clampi((int)g_cy, 0, g_gh - 1);
}

int32_t tb_DirectInput8Create(void *, uint32_t, const void *, TbDirectInput8 **out, void *) {
    if (!out) return (int32_t)0x80070057;   // E_INVALIDARG
    *out = new TbDirectInput8{ 1 };
    return 0;
}
void tb_DirectInput8_Release(TbDirectInput8 *di) { if (di && --di->refs <= 0) delete di; }
