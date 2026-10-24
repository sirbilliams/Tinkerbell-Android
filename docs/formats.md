# Tinker Bell data formats

Found from the files and the code that reads them (addresses are "Tinker Bell.exe" VAs). Reference decoders
in `tools/formats/` (Python, for checking; the port's C++ readers follow the game's code).

## Game folder (= `setup/game/` on the disc; copying it is the install)
`Tinker Bell.exe`, `data/` (71 `.wad`, the 3D scene files, 2 `.mov`), `audio/` (1121 `.wv6`), and
`Error Messages.dat`, `hiding_places.dat`, `ambient_sprites.dat`, `MagPies.dat` at the top level.
Display: 640x480, 16-bit (the game checks for RGB565 masks, 0x40d960).

## Records (`.plt` `.sp5` `.alf` `.wv6` `.rts` `.rtc`, standalone or inside a `.wad`)
Common 0x58-byte header: `u32 size` (whole record), `char name[40]` (file name), `char type[40]`
("Palette", "Sprite Image", "Alpha Channel", "Ray Trace Sprites", "Ray Trace", "WV6 IMA_ADPCM ..."),
`u32 version` (1). A `.wad` is just records back to back (all 71 parse to their exact length).
The loader (`Ras`, 0x419e30) looks for `<dir>\<sub>\<name>.<ext>` then `<dir>\<name>.<ext>`.

## Palette (`.plt`)
0x74 `u32 count` (256), 0x78 entries of 4 bytes **B, G, R, 0**.

## Sprite (`.sp5`), loaded by Pixelmap (0x416d70)
0x58..0x77: eight u32: flag, ?, ?, width (0x64), height (0x68), ?, bpp (0x70: 8 or 16), kind (0x74).
0x78 offset of the palette-name block, 0x7c offset of the alpha-name block, 0x80 offset of the pixel
block; each block starts with a u32 (count/length) and its data follows at offset+4.
16-bit pixels are RGB565. 8-bit pixels index the named palette. `kind & 0x10` = has an `.alf` alpha
record (exactly the 292 sprites that name one). `kind & ~0x10`:
- 0: raw rows.
- 1: per row, alternating transparent count / opaque count (u8), each opaque run followed by its pixels.
- 2: as 1, but a count byte 0xFF is followed by a u16 count (0x417850).
- 3: whole image as a byte stream: first byte = escape E; then `E, value, n` = n pixels of value,
  any other byte = one pixel. Pixels equal to E are transparent (Image::setHit, Image_Vision.c).
All 10,376 sprites decode to exactly their record length.

## Alpha channel (`.alf`)
Sprite-like header (8-bit, kind 1 in the sample). Exact use: with the blitters (Alpha_Vision.c).

## Sound (`.wv6`)
0x58 channels, 0x5c bits (16), 0x60 rate, 0x64 decoded byte length, 0x6c offset of the data block
(u32 length, then data). ADPCM decoded by 0x423170, **high nibble first**, IMA step/index tables
(0x513b70 / 0x513cd4, read from the exe), but the game's own delta: `t = step*(nib&7);
d = (t>>3)+(t>>2)`, sign = bit 3, clamp to 16 bits, index clamp 0..88. Keep this quirk.

## 3D scenes (Raytrace; exact fields with each scene in phase 2)
- `.rts` "Ray Trace Sprites": sprite names with positions (floats); `.rtc` "Ray Trace": wall names.
- `<scene>.txt`: wall segments, CSV `name,x1,z1,x2,z2,flag`.
- `.grn`: 8-byte header, u32 count, `count` ground-tile names (40 bytes each), then a 128x128 byte grid.
- `.spr`: 8-byte header + 128x128 byte grid.

## Text (`.dat`)
INI style (`[section]`, `key=value`, `//` comments, CRLF). `MagPies.dat`: per voice line, one letter
per frame (lip-sync mouth shapes).

## Movies (`.mov`, QuickTime)
`DIbumper.mov`: SVQ1 640x480 10 fps + PCM s16be 22050 mono. `LOND_MO_intro.mov`: RPZA 640x480 10 fps +
PCM s16be 44100 stereo. Played by QTMovie through QuickTime in the original; the port uses FFmpeg (LGPL,
shared libs, only the mov demuxer and the svq1/rpza/pcm decoders).
