# API call sites named from context (redirected `call [0x64b2f8]`)
Format: site, API, evidence. Lane C (text / GDI / input). `tb_*` = the platform shim function (platform/gdi.h, dinput.h).

## Text::v14 0x413bc0 (draws a Text onto the surface DC held in 0x5a688c, from IDirectDrawSurface::GetDC vtbl+0x44)
- 0x413bf1 SetTextColor(hdc, rgb)   (returns the old colour, restored at 0x413e75)
- 0x413bfe SetBkMode(hdc, 1)        (TRANSPARENT; old mode restored at 0x413e69)
- 0x413c1b SelectObject(hdc, font)  (font handle at Font+0x24; old font reselected at 0x413e5d)
- 0x413d12 CreatePen(6 = PS_INSIDEFRAME, border-1, colour); 0x413d28 GetStockObject(5 = NULL_BRUSH)
- 0x413d49 CreateBrushIndirect(&LOGBRUSH{0 = BS_SOLID, colour, 0}); 0x413d51 SelectObject(hdc, brush); 0x413d60 CreatePen(6, 0, colour); 0x413d68 SelectObject(hdc, pen)
- 0x413de3 Rectangle(hdc, l, t, r, b)   (virtual calls +0x1c..+0x28 give the four coordinates)
- 0x413df6 / 0x413e0d DeleteObject(previous pen / brush); 0x413e06 SelectObject(hdc, old pen)
- direct imps in the same function: SelectObject 0x413d20/0x413d30/0x413df3, DrawTextA 0x413e38 / 0x413e4f (flags 0x900 | align: NOPREFIX|NOCLIP, VCENTER 4 / BOTTOM 8, LEFT/CENTER/RIGHT; 0x910 = word break)

## Font object (vtable 0x51354c): 0x4124b0 create, 0x4125f0 destroy
- 0x4124be, 0x41262b DeleteObject(HFONT at +0x24)
- 0x4124e2 GetStockObject(0x10)     (no face name and no size)
- 0x4124f3, 0x41311c, 0x4133ac, 0x41344c, 0x4134cc, 0x4135ec, 0x413722, 0x4137c6, 0x413903, 0x413986 GetDC(0)
- 0x412583, 0x4125e0, 0x4132fd, 0x413400, 0x41348b, 0x413511, 0x413631, 0x413773, 0x41386c, 0x413952, 0x4139d5 ReleaseDC(0, hdc)
- 0x412500 GetDeviceCaps(hdc, 0x5a = LOGPIXELSY); 0x41250b MulDiv(size, logpixels, 0x48 = 72) -> -height
- 0x412578 CreateFontA(-h, 0, 0, +0x3c, +0x48 (weight), +0x44 italic, +0x45 underline, 0 x6, face)
- 0x412591 EnumFontFamiliesExA(hdc, &LOGFONT{face}, 0x412430 (callback: copies the LOGFONT whose lfFaceName at +0x1c matches), param, 0); 0x4125d4 CreateFontIndirectA(0x5a6988)

## Text measuring (GetDC(0), SelectObject(hdc, font), measure, SelectObject back, ReleaseDC)
- 0x41313b, 0x4133bf, 0x413735, 0x4137d9 SelectObject(hdc, font); 0x4132f5, 0x4133f7, 0x41376a, 0x413863 SelectObject(hdc, old)
- GetTextExtentPoint32A(hdc, str, len, &SIZE): 0x4131a3, 0x413203, 0x413264, 0x413287, 0x4133ef, 0x41347e, 0x413500, 0x413620, 0x413762, 0x413941, 0x4139c4
- 0x413827 GetTextExtentExPointA(hdc, str, count, 0, 0, int dx[count], &SIZE)   (cumulative widths; 0x4136d0 finds the character under an x)

## Input
- 0x4f1f4a DirectInput8Create(hInstance 0x5a6754, 0x800, IID_IDirectInput8A 0x517cf0, &0x5a6660, 0), service at 0x4182a0 (init) / 0x4182d0 (Release); no device is ever created.
- Window procedure 0x421760 gets WM_KEYDOWN/UP/CHAR (0x100-0x102), WM_MOUSEMOVE/L/R buttons (0x200-0x205) and calls GetKeyState(0x11 Ctrl, 0x10 Shift) at 0x4219a8/0x4219e9/0x421a33; key events go to 0x418410(vk, up, down, char, shift), mouse events to 0x418620(x, y, ...).
