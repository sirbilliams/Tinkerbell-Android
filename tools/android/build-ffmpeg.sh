#!/bin/bash
# Builds a minimal LGPL FFmpeg (shared) for arm64-v8a with the Android NDK, for the game's two .mov movies:
# the mov demuxer, the file protocol, the svq1 / rpza / pcm_s16be decoders, swscale and swresample.
# Output (what android/app/jni/CMakeLists.txt and build.gradle use):
#   android/app/jni/ffmpeg-inc/          headers
#   android/app/jni/ffmpeg-libs/arm64-v8a/lib{avutil,swresample,swscale,avcodec,avformat}.so (named by their sonames)
#   android/app/src/main/assets/licenses/FFmpeg-LGPL-2.1.txt
# The workflow caches android/app/jni/ffmpeg-inc + ffmpeg-libs, keyed on this script's and deps.lock's hash.
# Needs ANDROID_HOME with ndk;26.3.11579264 installed (or ANDROID_NDK_HOME).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "$ROOT/deps.lock"
JNI="$ROOT/android/app/jni"
API=24
NDK="${ANDROID_HOME:-}/ndk/26.3.11579264"          # the NDK the app builds with (build.gradle)
[ -d "$NDK" ] || NDK="${ANDROID_NDK_HOME:-}"
[ -d "$NDK" ] || { echo "ERROR: NDK not found (set ANDROID_NDK_HOME)" >&2; exit 1; }
TC="$NDK/toolchains/llvm/prebuilt/linux-x86_64"
LIBS="$JNI/ffmpeg-libs/arm64-v8a"

if ls "$LIBS"/libavcodec*.so >/dev/null 2>&1 && [ -f "$JNI/ffmpeg-inc/libavcodec/avcodec.h" ]; then
    echo "ok   ffmpeg already built"; exit 0
fi

SRC="$ROOT/build-ffmpeg-src"
rm -rf "$SRC"; git init -q "$SRC"
git -C "$SRC" remote add origin "$FFMPEG_REPO"
git -C "$SRC" fetch -q --depth 1 origin "refs/tags/$FFMPEG_REF"
git -C "$SRC" checkout -q FETCH_HEAD
got="$(git -C "$SRC" rev-parse HEAD)"
[ "$got" = "$FFMPEG_COMMIT" ] || { echo "ERROR: $FFMPEG_REPO $FFMPEG_REF is $got, expected $FFMPEG_COMMIT" >&2; exit 1; }
echo "got  ffmpeg @ ${got:0:10}"

OUT="$ROOT/build-ffmpeg-out"
rm -rf "$OUT"; mkdir -p "$OUT"
cd "$SRC"
./configure \
    --prefix="$OUT" --enable-cross-compile --target-os=android --arch=aarch64 --cpu=armv8-a \
    --cc="$TC/bin/aarch64-linux-android$API-clang" --cxx="$TC/bin/aarch64-linux-android$API-clang++" \
    --ar="$TC/bin/llvm-ar" --ranlib="$TC/bin/llvm-ranlib" --strip="$TC/bin/llvm-strip" --nm="$TC/bin/llvm-nm" \
    --sysroot="$TC/sysroot" \
    --disable-everything --disable-gpl --disable-nonfree --disable-version3 \
    --enable-shared --disable-static \
    --disable-programs --disable-doc --disable-debug --disable-avdevice --disable-avfilter --disable-network \
    --disable-autodetect --disable-asm \
    --enable-avformat --enable-avcodec --enable-swscale --enable-swresample \
    --enable-protocol=file --enable-demuxer=mov \
    --enable-decoder=svq1 --enable-decoder=rpza --enable-decoder=pcm_s16be \
    --extra-cflags="-O2 -fPIC"
# FFmpeg's android target names each library by its soname (libavformat.so, no version suffix) and links
# them to each other by those names, so they are installed and packaged under exactly those names: Android's
# loader looks dependencies up by soname in the APK's lib folder. (Renaming them broke loading on the Thor.)
make -j"$(nproc)"
make install

rm -rf "$JNI/ffmpeg-inc" "$JNI/ffmpeg-libs"
mkdir -p "$JNI/ffmpeg-inc" "$LIBS"
cp -r "$OUT/include/"* "$JNI/ffmpeg-inc/"
for l in avutil swresample swscale avcodec avformat; do cp "$OUT/lib/lib$l.so" "$LIBS/"; done
"$TC/bin/llvm-strip" --strip-unneeded "$LIBS"/*.so
mkdir -p "$ROOT/android/app/src/main/assets/licenses"
cp "$SRC/COPYING.LGPLv2.1" "$ROOT/android/app/src/main/assets/licenses/FFmpeg-LGPL-2.1.txt"
rm -rf "$SRC" "$OUT"
ls -la "$LIBS"
