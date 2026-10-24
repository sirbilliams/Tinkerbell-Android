// Movie playback shim (lane B): what QuickTime did for QTMovie (0x435810 open, 0x435a10 play, 0x435a50 stop,
// 0x435a70 isPlaying, 0x4359d0 dispose), on FFmpeg (desktop: system libav*; TB_FFMPEG=OFF builds a stub that
// reports no movie). Video is converted to RGB565 (the game's screen format); audio goes to the dsound mixer.
#pragma once
#include <stdint.h>

struct TbMovie;

TbMovie *tb_movie_open(const char *path);        // nullptr if it cannot be opened (or no FFmpeg)
void tb_movie_close(TbMovie *m);                 // dispose
void tb_movie_start(TbMovie *m);                 // from the beginning (GoToBeginningOfMovie + StartMovie)
void tb_movie_stop(TbMovie *m);                  // StopMovie
bool tb_movie_playing(TbMovie *m);               // false once the end has been played
// Decode ahead and advance the video to the clock (the soundtrack, or wall time when there is none).
// Call every tick (what MoviesTask did). Returns true when frame() changed.
bool tb_movie_task(TbMovie *m);
void tb_movie_size(TbMovie *m, int *w, int *h);
double tb_movie_time(TbMovie *m);                // seconds played
const uint16_t *tb_movie_frame(TbMovie *m);      // RGB565, w*h, row pitch = w*2; nullptr before the first frame
