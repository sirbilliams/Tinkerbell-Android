// GDI shim: the text and rectangle drawing the game does on a DirectDraw surface DC (user32 DrawTextA,
// gdi32 TextOutA / CreateFontA / ...). Lane C. Names are the Win32 ones with a tb_ prefix, argument order
// and semantics as Win32, so the reconstructed code calls them where the original called the API.
// port: Windows' fonts are replaced by the bundled Liberation Sans (SIL OFL), rendered with stb_truetype.
#pragma once
#include <stdint.h>
#include <stddef.h>

struct TbDC;                       // HDC
struct TbGdiObj;                   // HFONT / HPEN / HBRUSH / HGDIOBJ
typedef TbDC *TbHDC;
typedef TbGdiObj *TbHGDIOBJ;
typedef uint32_t TbCOLORREF;       // 0x00BBGGRR

struct TbRect { int32_t left, top, right, bottom; };      // RECT
struct TbSize { int32_t cx, cy; };                        // SIZE
struct TbPoint { int32_t x, y; };                         // POINT

struct TbLogFont {                                        // LOGFONTA (same layout, 60 bytes)
    int32_t lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
    uint8_t lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision, lfClipPrecision, lfQuality,
            lfPitchAndFamily;
    char lfFaceName[32];
};

struct TbTextMetric {                                     // TEXTMETRICA, the fields that matter
    int32_t tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading, tmAveCharWidth, tmMaxCharWidth,
            tmWeight, tmOverhang, tmDigitizedAspectX, tmDigitizedAspectY;
    uint8_t tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar, tmItalic, tmUnderlined, tmStruckOut,
            tmPitchAndFamily, tmCharSet;
};

// ---- device contexts
// The pixels a DC draws on: the DirectDraw surface's memory (lane A's shim passes it from IDirectDrawSurface::GetDC).
struct TbSurfaceBits {
    void *pixels;                  // top-left pixel
    int pitch;                     // bytes per row
    int width, height;
    int bpp;                       // 16 or 32
    uint32_t rmask, gmask, bmask;  // pixel format masks (RGB565: f800/07e0/001f; 32 bpp: ff0000/00ff00/0000ff)
};
TbHDC tb_gdi_surface_dc(const TbSurfaceBits &bits);   // GetDC on a surface; free with tb_gdi_release_surface_dc
void tb_gdi_release_surface_dc(TbHDC dc);             // IDirectDrawSurface::ReleaseDC

TbHDC tb_GetDC(void *hwnd);                    // user32 GetDC(0): a screen DC; text can be measured, drawing is dropped
int tb_ReleaseDC(void *hwnd, TbHDC dc);        // user32 ReleaseDC

// ---- objects
enum {
    TB_WHITE_BRUSH = 0, TB_LTGRAY_BRUSH = 1, TB_GRAY_BRUSH = 2, TB_DKGRAY_BRUSH = 3, TB_BLACK_BRUSH = 4,
    TB_NULL_BRUSH = 5, TB_WHITE_PEN = 6, TB_BLACK_PEN = 7, TB_NULL_PEN = 8, TB_OEM_FIXED_FONT = 10,
    TB_ANSI_FIXED_FONT = 11, TB_ANSI_VAR_FONT = 12, TB_SYSTEM_FONT = 13, TB_DEVICE_DEFAULT_FONT = 14,
    TB_DEFAULT_PALETTE = 15, TB_SYSTEM_FIXED_FONT = 16, TB_DEFAULT_GUI_FONT = 17,
};
struct TbLogBrush { uint32_t lbStyle; TbCOLORREF lbColor; intptr_t lbHatch; };   // LOGBRUSH (BS_SOLID = 0, BS_NULL = 1)

TbHGDIOBJ tb_CreateFontA(int h, int w, int esc, int orient, int weight, uint32_t italic, uint32_t underline,
                         uint32_t strike, uint32_t charset, uint32_t outprec, uint32_t clipprec, uint32_t quality,
                         uint32_t pitchfam, const char *face);
TbHGDIOBJ tb_CreateFontIndirectA(const TbLogFont *lf);
TbHGDIOBJ tb_CreatePen(int style, int width, TbCOLORREF color);
TbHGDIOBJ tb_CreateBrushIndirect(const TbLogBrush *lb);
TbHGDIOBJ tb_GetStockObject(int index);
bool tb_DeleteObject(TbHGDIOBJ obj);           // stock objects are left alone
TbHGDIOBJ tb_SelectObject(TbHDC dc, TbHGDIOBJ obj);    // returns the previous object of that kind

// ---- state
TbCOLORREF tb_SetTextColor(TbHDC dc, TbCOLORREF c);    // returns the previous colour
int tb_SetBkMode(TbHDC dc, int mode);                  // TRANSPARENT = 1, OPAQUE = 2; returns the previous mode
bool tb_SetViewportOrgEx(TbHDC dc, int x, int y, TbPoint *old);
int tb_GetDeviceCaps(TbHDC dc, int index);             // LOGPIXELSY (90) / LOGPIXELSX (88): 96; HORZRES 8 / VERTRES 10: the surface
static inline int tb_MulDiv(int a, int b, int c) {     // kernel32 MulDiv: rounds to nearest, -1 if c == 0
    if (c == 0) return -1;
    long long p = (long long)a * b, h = c / 2;
    p += (p >= 0) == (c > 0) ? h : -h;
    return (int)(p / c);
}

// ---- drawing
bool tb_TextOutA(TbHDC dc, int x, int y, const char *s, int len);
// DT_ flags: LEFT 0, CENTER 1, RIGHT 2, VCENTER 4, BOTTOM 8, WORDBREAK 0x10, SINGLELINE 0x20, EXPANDTABS 0x40,
// NOCLIP 0x100, CALCRECT 0x400, NOPREFIX 0x800. Returns the text height; with CALCRECT only measures (rect updated).
int tb_DrawTextA(TbHDC dc, const char *s, int len, TbRect *rc, uint32_t flags);
bool tb_Rectangle(TbHDC dc, int l, int t, int r, int b);   // current pen outline + brush fill
bool tb_GetTextExtentPoint32A(TbHDC dc, const char *s, int len, TbSize *sz);
bool tb_GetTextExtentExPointA(TbHDC dc, const char *s, int len, int maxWidth, int *fit, int *dx, TbSize *sz);
// Calls the callback once, with a LOGFONT carrying the requested face name (every face "exists": it maps to the bundled font).
typedef int (*TbFontEnumProc)(const TbLogFont *lf, const TbTextMetric *tm, uint32_t fontType, intptr_t lparam);
int tb_EnumFontFamiliesExA(TbHDC dc, const TbLogFont *lf, TbFontEnumProc proc, intptr_t lparam, uint32_t flags);
