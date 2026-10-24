// Vision::QTMovie (lane B), reconstructed from raw/QTMovie.c and 0x435730 (ctor), 0x435810 (open),
// 0x4359d0 (dispose), 0x435a10 (play), 0x435a50 (stop), 0x435a70 (isPlaying).
// The original drove QuickTime (selectors 0xf00e2 NativePathNameToFSSpec, 0x20192 OpenMovieFile,
// 0x200f0 NewMovieFromFile, 0x200d5 CloseMovieFile, 0x2000d GoToBeginningOfMovie, 0x2000b StartMovie,
// 0x2000c StopMovie, 0x20023 DisposeMovie); the port uses platform/movie.h (FFmpeg).
#pragma once
#include "../../../platform/movie.h"
#include <stdint.h>
#include <string>

namespace Vision {

class QTMovie {
public:
    QTMovie(const char *name, const char *dir = "data");   // 0x435730 ("data" is what MovieScene passes)
    ~QTMovie();                                            // 0x4357f0

    std::string name, dir;
    void open();                    // 0x435810: "<dir>\<name>.mov"
    void dispose();                 // 0x4359d0
    void play();                    // 0x435a10
    void stop();                    // 0x435a50
    bool isPlaying();               // 0x435a70
    // port: QuickTime ran itself; the caller ticks the movie (Service tick / the MovieScene loop) and draws frame()
    bool task();
    const uint16_t *frame();        // RGB565 640x480 (size()), nullptr until the first frame is ready
    void size(int *w, int *h);
    bool loaded() const { return m_ != nullptr; }

private:
    TbMovie *m_;
    bool playing_;                  // *(char *)(+0x34)
};

}  // namespace Vision
