# Third-party software
- SDL2 (zlib license), fetched at build time.
- FFmpeg (LGPL 2.1+), dynamically linked, only the decoders the game's two movies need (planned).
Tools (not shipped): radare2 / r2ghidra, capstone, pefile, pycdlib.
- stb_truetype (public domain / MIT, Sean Barrett), platform/third_party/stb_truetype.h: glyph rasteriser for the GDI text shim.
- Liberation Sans (SIL Open Font License 1.1), platform/fonts/: the bundled font that stands in for the game's Windows fonts (license text in platform/fonts/LICENSE-Liberation.txt).
