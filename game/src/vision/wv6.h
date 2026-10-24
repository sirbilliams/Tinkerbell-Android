// Vision .wv6 sound record + the game's IMA-ADPCM decoder (lane B).
// Reconstructed from 0x422d60 (ctor), 0x422f30 (parse a record in memory), 0x423060 (read the header from a
// file), 0x423170 (decoder), 0x423490/0x423520/0x423540/0x423560/0x423640 (readers).
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <vector>

namespace Vision {

class Wv6 {
public:
    Wv6();                                   // 0x422d60: tables copied from the exe (0x513b70 / 0x513cd4)
    ~Wv6();

    // ---- record header (offsets in the .wv6 record)
    uint32_t recSize;                        // 0x00
    char name[40];                           // 0x04
    char type[40];                           // 0x2c
    uint32_t version;                        // 0x54
    uint32_t channels;                       // 0x58
    uint32_t bits;                           // 0x5c
    uint32_t rate;                           // 0x60
    uint32_t bytes;                          // 0x64  decoded length in bytes
    uint32_t compressed;                     // 0x68  0 = raw PCM
    uint32_t dataOfs;                        // 0x6c  file offset of the data block's length word (0x88)
    uint32_t has2, has3;                     // 0x70, 0x74: a second / third block follows the data
    uint32_t h78, h7c, h80, h84;             // 0x78..0x84 (not used by the sound code)
    uint32_t dataLen;                        // 0x88  length of the data block

    // ---- data blocks (pointers into the record the caller keeps alive)
    const uint8_t *data;  uint32_t len1;     // block 1 (the ADPCM data)
    const uint8_t *data2; uint32_t len2;     // block 2 (present when has2)
    const uint8_t *data3; uint32_t len3;     // block 3 (present when has3)

    bool loop;                               // 0x25c: the stream wraps instead of going silent

    bool parse(const uint8_t *rec);          // 0x422f30
    bool readHeader(FILE *f);                // 0x423060 (stream-from-file: header only)
    void reset();                            // 0x423540: position 0, step index 0, predictor 0
    uint32_t frames() const;                 // 0x423520
    void readAll(void *out);                 // 0x423490: whole sound into out (bytes bytes)
    bool readMem(int16_t *out, uint32_t count);                       // 0x423560
    bool readFile(int16_t *out, uint32_t count, FILE *f);             // 0x423640
    void decode(int16_t *out, const uint8_t *src, uint32_t start, uint32_t count);   // 0x423170

private:
    void clear();                            // 0x422e10
    int32_t step_[89];                       // 0x2c.. (89 dwords from 0x513b70)
    int32_t indexAdj_[8];                    // 0x190.. (8 dwords from 0x513cd4)
    int32_t index_, pred_;                   // 0x08, 0x0c
    bool pending_;                           // 0x28: the next sample is the low nibble of the byte at the read pointer
    uint32_t pos_;                           // 0x1b8
    std::vector<uint8_t> tmp_;               // 0x1b4: compressed bytes read from the file
    void sample(int16_t *&o, uint32_t nib);
};

}  // namespace Vision
