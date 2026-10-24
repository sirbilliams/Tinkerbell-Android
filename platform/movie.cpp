// Movie playback on FFmpeg (lane B). See movie.h.
#include "movie.h"
#include "dsound.h"
#include "tb.h"

#ifdef TB_HAVE_FFMPEG
#include <SDL.h>
#include <deque>
#include <vector>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace {
struct VFrame { double pts; AVFrame *f; };
}

struct TbMovie {
    AVFormatContext *fc = nullptr;
    int vi = -1, ai = -1;
    AVCodecContext *vc = nullptr, *ac = nullptr;
    SwsContext *sws = nullptr;
    SwrContext *swr = nullptr;
    AVPacket *pkt = nullptr;
    AVFrame *frm = nullptr;
    std::deque<VFrame> vq;
    std::vector<uint16_t> rgb;
    int w = 0, h = 0;
    double fps = 10, lastPts = 0, nextPts = 0;
    TbPcmStream *pcm = nullptr;
    int outRate = 44100;
    bool playing = false, eof = false, flushed = false, haveFrame = false, audioDone = false;
    uint32_t wall0 = 0;            // SDL ticks at start
    double audioEndClock = -1;     // clock value when the soundtrack ran dry, then wall time continues from it
    uint32_t wallAtAudioEnd = 0;
    double duration = 0;
};

static double vtb(TbMovie *m) { return av_q2d(m->fc->streams[m->vi]->time_base); }

static void push_audio(TbMovie *m, AVFrame *f) {
    if (!m->pcm) return;
    if (!m->swr) {
        AVChannelLayout out;
        av_channel_layout_default(&out, 2);
        if (swr_alloc_set_opts2(&m->swr, &out, AV_SAMPLE_FMT_S16, m->outRate, &m->ac->ch_layout, m->ac->sample_fmt,
                                m->ac->sample_rate, 0, nullptr) < 0 || swr_init(m->swr) < 0) {
            tb_log("movie: cannot set up the audio converter");
            swr_free(&m->swr);
            tb_pcmstream_destroy(m->pcm); m->pcm = nullptr;
            return;
        }
    }
    int maxOut = swr_get_out_samples(m->swr, f ? f->nb_samples : 0) + 64;
    std::vector<int16_t> buf((size_t)maxOut * 2);
    uint8_t *op[1] = { (uint8_t *)buf.data() };
    int n = swr_convert(m->swr, op, maxOut, f ? (const uint8_t **)f->extended_data : nullptr, f ? f->nb_samples : 0);
    if (n > 0) tb_pcmstream_push(m->pcm, buf.data(), n);
}

static void drain_video(TbMovie *m) {
    while (avcodec_receive_frame(m->vc, m->frm) == 0) {
        double pts;
        if (m->frm->best_effort_timestamp != AV_NOPTS_VALUE) pts = m->frm->best_effort_timestamp * vtb(m);
        else pts = m->nextPts;
        m->nextPts = pts + 1.0 / m->fps;
        VFrame v; v.pts = pts; v.f = av_frame_clone(m->frm);
        m->vq.push_back(v);
        av_frame_unref(m->frm);
    }
}

static void drain_audio(TbMovie *m) {
    while (avcodec_receive_frame(m->ac, m->frm) == 0) {
        push_audio(m, m->frm);
        av_frame_unref(m->frm);
    }
}

// Read and decode packets until enough is queued ahead.
static void pump(TbMovie *m) {
    const int wantAudio = m->outRate / 2;
    while (!m->eof) {
        bool needA = m->pcm && m->ai >= 0 && tb_pcmstream_queued(m->pcm) < wantAudio;
        bool needV = m->vq.size() < 4;
        if (!needA && !needV) break;
        int r = av_read_frame(m->fc, m->pkt);
        if (r < 0) {
            m->eof = true;
            if (!m->flushed) {
                m->flushed = true;
                avcodec_send_packet(m->vc, nullptr); drain_video(m);
                if (m->ac) { avcodec_send_packet(m->ac, nullptr); drain_audio(m); push_audio(m, nullptr); }
            }
            break;
        }
        if (m->pkt->stream_index == m->vi) { avcodec_send_packet(m->vc, m->pkt); drain_video(m); }
        else if (m->pkt->stream_index == m->ai && m->ac) { avcodec_send_packet(m->ac, m->pkt); drain_audio(m); }
        av_packet_unref(m->pkt);
    }
}

static void free_vq(TbMovie *m) {
    for (size_t i = 0; i < m->vq.size(); i++) av_frame_free(&m->vq[i].f);
    m->vq.clear();
}

TbMovie *tb_movie_open(const char *path) {
    TbMovie *m = new TbMovie();
    if (avformat_open_input(&m->fc, path, nullptr, nullptr) < 0) { tb_log("movie: cannot open %s", path); delete m; return nullptr; }
    avformat_find_stream_info(m->fc, nullptr);
    m->vi = av_find_best_stream(m->fc, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    m->ai = av_find_best_stream(m->fc, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m->vi < 0) { tb_log("movie: %s has no video", path); tb_movie_close(m); return nullptr; }
    for (int k = 0; k < 2; k++) {
        int idx = k == 0 ? m->vi : m->ai;
        if (idx < 0) continue;
        AVCodecParameters *par = m->fc->streams[idx]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(par->codec_id);
        AVCodecContext *c = codec ? avcodec_alloc_context3(codec) : nullptr;
        if (!c || avcodec_parameters_to_context(c, par) < 0 || avcodec_open2(c, codec, nullptr) < 0) {
            tb_log("movie: no decoder for stream %d of %s", idx, path);
            avcodec_free_context(&c);
            if (k == 0) { tb_movie_close(m); return nullptr; }
            m->ai = -1;
            continue;
        }
        (k == 0 ? m->vc : m->ac) = c;
    }
    m->w = m->vc->width; m->h = m->vc->height;
    AVRational fr = m->fc->streams[m->vi]->avg_frame_rate;
    if (fr.num > 0 && fr.den > 0) m->fps = av_q2d(fr);
    m->duration = m->fc->duration > 0 ? (double)m->fc->duration / AV_TIME_BASE : 0;
    m->rgb.assign((size_t)m->w * m->h, 0);
    m->pkt = av_packet_alloc();
    m->frm = av_frame_alloc();
    m->outRate = tb_dsound_device_rate();
    tb_log("movie: %s %dx%d %.1f fps, audio %s", path, m->w, m->h, m->fps, m->ai >= 0 ? "yes" : "no");
    return m;
}

void tb_movie_close(TbMovie *m) {
    if (!m) return;
    free_vq(m);
    if (m->pcm) tb_pcmstream_destroy(m->pcm);
    sws_freeContext(m->sws);
    swr_free(&m->swr);
    av_packet_free(&m->pkt);
    av_frame_free(&m->frm);
    avcodec_free_context(&m->vc);
    avcodec_free_context(&m->ac);
    avformat_close_input(&m->fc);
    delete m;
}

void tb_movie_start(TbMovie *m) {
    if (!m) return;
    if (m->pcm) { tb_pcmstream_destroy(m->pcm); m->pcm = nullptr; }
    free_vq(m);
    av_seek_frame(m->fc, -1, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m->vc);
    if (m->ac) avcodec_flush_buffers(m->ac);
    swr_free(&m->swr);
    m->eof = m->flushed = m->haveFrame = false;
    m->nextPts = 0;
    m->audioEndClock = -1;
    m->outRate = tb_dsound_device_rate();
    if (m->ai >= 0 && g_tb_dsound) m->pcm = tb_pcmstream_create();
    m->wall0 = SDL_GetTicks();
    m->playing = true;
    pump(m);
}

void tb_movie_stop(TbMovie *m) {
    if (!m) return;
    m->playing = false;
    if (m->pcm) tb_pcmstream_clear(m->pcm);
}

static double clock_now(TbMovie *m) {
    double wall = (SDL_GetTicks() - m->wall0) / 1000.0;
    if (!m->pcm) return wall;
    double a = (double)tb_pcmstream_played(m->pcm) / m->outRate;
    if (m->audioEndClock >= 0) return m->audioEndClock + (SDL_GetTicks() - m->wallAtAudioEnd) / 1000.0;
    if (m->eof && tb_pcmstream_queued(m->pcm) == 0) {      // the soundtrack ran dry: carry on by wall time
        m->audioEndClock = a; m->wallAtAudioEnd = SDL_GetTicks();
    }
    return a;
}

double tb_movie_time(TbMovie *m) { return m ? clock_now(m) : 0; }

bool tb_movie_task(TbMovie *m) {
    if (!m || !m->playing) return false;
    pump(m);
    double t = clock_now(m);
    VFrame show = { 0, nullptr };
    while (!m->vq.empty() && m->vq.front().pts <= t + 1e-4) {
        if (show.f) av_frame_free(&show.f);
        show = m->vq.front();
        m->vq.pop_front();
        m->lastPts = show.pts;
    }
    bool changed = false;
    if (show.f) {
        m->sws = sws_getCachedContext(m->sws, show.f->width, show.f->height, (AVPixelFormat)show.f->format,
                                      m->w, m->h, AV_PIX_FMT_RGB565LE, SWS_BILINEAR, nullptr, nullptr, nullptr);
        uint8_t *dst[1] = { (uint8_t *)m->rgb.data() };
        int ds[1] = { m->w * 2 };
        sws_scale(m->sws, show.f->data, show.f->linesize, 0, show.f->height, dst, ds);
        av_frame_free(&show.f);
        m->haveFrame = changed = true;
    }
    bool audioLeft = m->pcm && tb_pcmstream_queued(m->pcm) > 0;
    if (m->eof && m->vq.empty() && !audioLeft && t >= m->lastPts + 1.0 / m->fps) m->playing = false;
    return changed;
}

bool tb_movie_playing(TbMovie *m) { return m && m->playing; }
void tb_movie_size(TbMovie *m, int *w, int *h) { *w = m ? m->w : 0; *h = m ? m->h : 0; }
const uint16_t *tb_movie_frame(TbMovie *m) { return m && m->haveFrame ? m->rgb.data() : nullptr; }

#else   // no FFmpeg: movies are unavailable; the game skips them (QTMovie::open fails)
struct TbMovie {};
TbMovie *tb_movie_open(const char *) { tb_log("movie: built without FFmpeg (TB_FFMPEG=OFF)"); return nullptr; }
void tb_movie_close(TbMovie *) {}
void tb_movie_start(TbMovie *) {}
void tb_movie_stop(TbMovie *) {}
bool tb_movie_playing(TbMovie *) { return false; }
bool tb_movie_task(TbMovie *) { return false; }
void tb_movie_size(TbMovie *, int *w, int *h) { *w = 0; *h = 0; }
double tb_movie_time(TbMovie *) { return 0; }
const uint16_t *tb_movie_frame(TbMovie *) { return nullptr; }
#endif
