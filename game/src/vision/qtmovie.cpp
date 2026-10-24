#include "qtmovie.h"
#include "../../../platform/dsound.h"
#include "tb.h"

namespace Vision {

QTMovie::QTMovie(const char *n, const char *d)   // 0x435730
    : Service(nullptr), movieName(n ? n : ""), movieDir(d ? d : ""), m_(nullptr), playing_(false) {}

Gorilla::Node *QTMovie::root() { static Gorilla::Node n; return &n; }   // 0x435b90 (0x5a9630)

QTMovie::~QTMovie() { dispose(); }              // 0x4357f0

void QTMovie::open() {                          // 0x435810
    if (m_) return;
    std::string rel = movieDir.empty() ? movieName + ".mov" : movieDir + "\\" + movieName + ".mov";
    // port: QuickTime opened the file by FSSpec; here the path is resolved case-insensitively and opened by FFmpeg
    std::string full = tb_game_path(rel.c_str());
    FILE *f = tb_fopen_ci(rel.c_str());
    if (f) fclose(f); else { tb_log("QTMovie: cannot open %s", rel.c_str()); return; }
    m_ = tb_movie_open(full.c_str());
    if (!m_) tb_log("QTMovie: %s could not be loaded", rel.c_str());
}

void QTMovie::dispose() {                       // 0x4359d0
    if (!m_) return;
    if (playing_) { tb_movie_stop(m_); playing_ = false; }
    deactivate();                               // +0x1c
    tb_movie_close(m_);
    m_ = nullptr;
}

void QTMovie::play() {                          // 0x435a10
    if (!playing_ && m_) {
        playing_ = true;
        tb_movie_start(m_);                     // GoToBeginningOfMovie, StartMovie
        activate();                             // +0x18
    }
}

void QTMovie::stop() {                          // 0x435a50
    if (playing_ && m_) { tb_movie_stop(m_); playing_ = false; }
}

bool QTMovie::isPlaying() {                     // 0x435a70
    // port: QuickTime cleared playback by itself at the end of the movie
    if (playing_ && m_ && !tb_movie_playing(m_)) playing_ = false;
    return playing_;
}

bool QTMovie::task() { return m_ && playing_ ? tb_movie_task(m_) : false; }
const uint16_t *QTMovie::frame() { return m_ ? tb_movie_frame(m_) : nullptr; }
void QTMovie::size(int *w, int *h) { if (m_) tb_movie_size(m_, w, h); else { *w = 0; *h = 0; } }

}  // namespace Vision
