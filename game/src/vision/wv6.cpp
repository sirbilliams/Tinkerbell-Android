#include "wv6.h"
#include "tb.h"
#include <string.h>

namespace Vision {

Wv6::Wv6() {                                 // 0x422d60
    clear();
    const int32_t *st = (const int32_t *)tb_exe_ptr(0x513b70, sizeof step_);
    const int32_t *ix = (const int32_t *)tb_exe_ptr(0x513cd4, sizeof indexAdj_);
    if (st) memcpy(step_, st, sizeof step_); else memset(step_, 0, sizeof step_);
    if (ix) memcpy(indexAdj_, ix, sizeof indexAdj_); else memset(indexAdj_, 0, sizeof indexAdj_);
    loop = false;
}

Wv6::~Wv6() {}

void Wv6::clear() {                          // 0x422e10
    recSize = 0; memset(name, 0, sizeof name); memset(type, 0, sizeof type);
    version = channels = bits = rate = bytes = compressed = dataOfs = 0;
    has2 = has3 = h78 = h7c = h80 = h84 = dataLen = 0;
    data = data2 = data3 = nullptr; len1 = len2 = len3 = 0;
    index_ = 0; pred_ = 0; pending_ = false; pos_ = 0;
    tmp_.clear();
}

static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

bool Wv6::parse(const uint8_t *rec) {        // 0x422f30
    recSize = rd32(rec);
    memcpy(name, rec + 4, 40);
    memcpy(type, rec + 0x2c, 40);
    uint32_t *f[] = { &version, &channels, &bits, &rate, &bytes, &compressed, &dataOfs, &has2, &has3, &h78, &h7c, &h80, &h84 };
    for (int i = 0; i < 13; i++) *f[i] = rd32(rec + 0x54 + i * 4);
    dataLen = rd32(rec + 0x88);
    data = rec + 0x8c; len1 = dataLen;
    const uint8_t *p = data + len1;
    data2 = data3 = nullptr; len2 = len3 = 0;
    if (has2) { len2 = rd32(p); data2 = p + 4; p = data2 + len2; }
    if (has3) { len3 = rd32(p); data3 = p + 4; }
    return true;
}

bool Wv6::readHeader(FILE *f) {              // 0x423060
    uint8_t h[0x88];
    if (fread(h, 1, sizeof h, f) != sizeof h) return false;
    data = data2 = data3 = nullptr; len1 = len2 = len3 = 0;
    recSize = rd32(h);
    memcpy(name, h + 4, 40);
    memcpy(type, h + 0x2c, 40);
    uint32_t *fld[] = { &version, &channels, &bits, &rate, &bytes, &compressed, &dataOfs, &has2, &has3, &h78, &h7c, &h80, &h84 };
    for (int i = 0; i < 13; i++) *fld[i] = rd32(h + 0x54 + i * 4);
    return true;
}

void Wv6::reset() {                          // 0x423540
    pos_ = 0; index_ = 0; pred_ = 0; pending_ = false;
}

uint32_t Wv6::frames() const {               // 0x423520
    if (bytes == 0) return 0;
    return bytes / (channels << 1);
}

void Wv6::sample(int16_t *&o, uint32_t nib) {
    // 0x423170: the game's own delta, t = step*(nib&7); d = ((t>>3)+(t>>2)) * sign
    int32_t sign = 1 - (((int32_t)nib >> 2) & 2);
    uint32_t mag = nib & 7;
    int32_t t = step_[index_] * (int32_t)mag;
    int32_t d = ((t >> 3) + (t >> 2)) * sign;
    int32_t p = pred_ + d;
    if (p >= 0x8000) p = 0x7fff; else if (p < -0x8000) p = -0x8000;
    pred_ = p;
    *o++ = (int16_t)p;
    int32_t ix = index_ + indexAdj_[mag];
    if (ix < 0) ix = 0; else if (ix > 0x58) ix = 0x58;
    index_ = ix;
}

void Wv6::decode(int16_t *out, const uint8_t *src, uint32_t start, uint32_t count) {   // 0x423170
    if (count == 0) return;
    uint32_t pairs = count >> 1;
    const uint8_t *p = src + (start >> 1);
    int16_t *o = out;
    uint32_t first = 0;
    if (pending_) {                          // finish the byte the previous call stopped in
        pending_ = false;
        sample(o, *p & 0xf);
        p++;
        first = 1;
    }
    uint32_t total = first + pairs * 2;
    if (count < total) { total -= 2; pairs--; }
    while (pairs) {                          // high nibble first
        sample(o, *p >> 4);
        sample(o, *p & 0xf);
        p++;
        pairs--;
    }
    if (total < count) {
        pending_ = true;
        sample(o, *p >> 4);
    }
}

void Wv6::readAll(void *out) {               // 0x423490
    if (!data) return;
    if (compressed == 0) { memcpy(out, data, bytes); return; }
    pos_ = 0; index_ = 0; pred_ = 0; pending_ = false;
    if (bytes == 0) { decode((int16_t *)out, data, 0, 0); return; }
    decode((int16_t *)out, data, 0, bytes / (channels << 1));
}

bool Wv6::readMem(int16_t *out, uint32_t count) {   // 0x423560
    uint32_t remaining = frames() - pos_;
    if (count < remaining) {
        decode(out, data, pos_, count);
        pos_ += count;
        return true;
    }
    decode(out, data, pos_, remaining);
    count -= remaining;
    int16_t *o2 = out + remaining;
    if (loop) {
        pos_ = 0; index_ = 0; pred_ = 0; pending_ = false;
        decode(o2, data, 0, count);
        pos_ = count;
        return false;
    }
    memset(o2, 0, (size_t)count * 2);
    pos_ += remaining;
    return false;
}

bool Wv6::readFile(int16_t *out, uint32_t count, FILE *f) {   // 0x423640
    uint32_t remaining = frames() - pos_;
    if (tmp_.empty()) tmp_.resize((count >> 1) + 1);
    if (remaining == frames()) {             // at the start: rewind to the data
        pos_ = 0; index_ = 0; pred_ = 0; pending_ = false;
        fseek(f, (long)(dataOfs + 4), SEEK_SET);
    }
    uint32_t half = count >> 1, odd = count & 1;
    if (remaining < count) { half = remaining >> 1; odd = remaining & 1; }
    if (pending_) odd = 1;
    if (tmp_.size() < half + odd) tmp_.resize(half + odd);   // port: the original's buffer was sized once, by the first call
    size_t got = fread(&tmp_[0], 1, half + odd, f);
    if (got < half + odd) memset(&tmp_[got], 0, half + odd - got);   // port: short file reads as silence-ish zeros
    // 0x423706: whenever a byte is shared with the next call the file is moved back one byte. This is the
    // game's own behaviour, also when the pending nibble was just finished (the next read repeats a byte).
    if (odd) fseek(f, -1, SEEK_CUR);
    if (remaining > count) {
        decode(out, &tmp_[0], 0, count);
        pos_ += count;
        return true;
    }
    decode(out, &tmp_[0], 0, remaining);
    uint32_t left = count - remaining;
    uint32_t odd2 = left & 1;
    int16_t *o2 = out + remaining;
    if (loop) {
        fseek(f, (long)(dataOfs + 4), SEEK_SET);
        uint32_t n = (left >> 1) + odd2;
        if (tmp_.size() < n) tmp_.resize(n);
        got = fread(&tmp_[0], 1, n, f);
        if (got < n) memset(&tmp_[got], 0, n - got);
        if (odd2) fseek(f, -1, SEEK_CUR);
        pos_ = 0; index_ = 0; pred_ = 0; pending_ = false;
        decode(o2, &tmp_[0], 0, left);
        pos_ = left;
        return false;
    }
    memset(o2, 0, (size_t)left * 2);
    pos_ += remaining;
    return false;
}

}  // namespace Vision
