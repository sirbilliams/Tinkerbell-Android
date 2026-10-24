# Lane C: what the other lanes call
- Input: `platform/dinput.h` (header comment has the loop). Mouse/Keyboard/Joystick (Vision::) are empty Service shells in
  the exe (vtables 0x513434 / 0x513408 / 0x515644: only class-info, ctor and dtor; Joystick is never constructed), so the
  real input path is the window procedure 0x421760 fed by `tb_input_pop`.
- Text: `platform/gdi.h`. `IDirectDrawSurface::GetDC` -> `tb_gdi_surface_dc(bits)`, `ReleaseDC` -> `tb_gdi_release_surface_dc`;
  user32 `GetDC(0)` / `ReleaseDC(0, dc)` -> `tb_GetDC(0)` / `tb_ReleaseDC`. Font: bundled Liberation Sans (platform/fonts, SIL OFL).
- Android: `android/` (SetupActivity installs setup/game from the ISO, GameActivity sets TB_ANDROID_GAME_DIR), `platform/crash_android.cpp`.
