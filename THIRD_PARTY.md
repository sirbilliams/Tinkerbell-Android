# Third-party software
- SDL2 (zlib license), fetched at build time.
- FFmpeg 6.1.1 (LGPL 2.1+, https://ffmpeg.org), dynamically linked (shared libavformat/libavcodec/libswscale/libswresample/libavutil). Built without GPL or nonfree parts and with everything disabled except the mov demuxer, file protocol and the svq1/rpza/pcm_s16be decoders: tools/android/build-ffmpeg.sh (Android); the system libraries on desktop. The app ships the license text (assets/licenses/FFmpeg-LGPL-2.1.txt) and the exact source is the pinned tag in deps.lock.
Tools (not shipped): radare2 / r2ghidra, capstone, pefile, pycdlib.
- stb_truetype (public domain / MIT, Sean Barrett), platform/third_party/stb_truetype.h: glyph rasteriser for the GDI text shim.
- Liberation Sans (SIL Open Font License 1.1), platform/fonts/: the bundled font that stands in for the game's Windows fonts (license text in platform/fonts/LICENSE-Liberation.txt).
