// Reads initial values the game needs (tables, strings, defaults) from the player's own
// "Tinker Bell.exe" at run time, so no bytes of the exe ever live in this repo.
//
// Only the game's plain sections are served: .rdata, .data (with its zero-initialised tail) and
// .text outside the CRT pages that the retail copy-protection wrapper keeps encrypted. The
// wrapper's own sections (.cms_t/.cms_d) are never read.
#include "tb.h"
#include <stdio.h>
#include <string.h>
#include <vector>

namespace {
struct Section { std::string name; uint32_t va, size; std::vector<uint8_t> bytes; };
std::vector<Section> g_secs;
bool g_loaded = false;

const uint32_t kEncLo = 0x4f3000, kEncHi = 0x4f8000;   // encrypted CRT pages: not served

uint32_t rd32(const std::vector<uint8_t> &f, size_t o) {
    return o + 4 <= f.size() ? (uint32_t)f[o] | (uint32_t)f[o + 1] << 8 | (uint32_t)f[o + 2] << 16 | (uint32_t)f[o + 3] << 24 : 0;
}
uint16_t rd16(const std::vector<uint8_t> &f, size_t o) { return o + 2 <= f.size() ? (uint16_t)(f[o] | f[o + 1] << 8) : 0; }
}

bool tb_exe_load() {
    if (g_loaded) return true;
    std::string path = tb_game_path("Tinker Bell.exe");
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) { tb_log("exe: cannot open %s", path.c_str()); return false; }
    std::vector<uint8_t> f;
    uint8_t buf[65536]; size_t n;
    while ((n = fread(buf, 1, sizeof buf, fp)) > 0) f.insert(f.end(), buf, buf + n);
    fclose(fp);
    uint32_t pe = rd32(f, 0x3c);
    if (rd32(f, pe) != 0x4550) { tb_log("exe: %s is not a PE file", path.c_str()); return false; }
    uint16_t nsec = rd16(f, pe + 6), optsz = rd16(f, pe + 20);
    uint32_t base = rd32(f, pe + 24 + 28);
    size_t sh = pe + 24 + optsz;
    for (int i = 0; i < nsec; i++, sh += 40) {
        char nm[9] = {0};
        if (sh + 40 > f.size()) break;
        memcpy(nm, &f[sh], 8);
        Section s; s.name = nm;
        if (s.name != ".text" && s.name != ".rdata" && s.name != ".data") continue;
        uint32_t vsize = rd32(f, sh + 8), rva = rd32(f, sh + 12), rawsz = rd32(f, sh + 16), rawoff = rd32(f, sh + 20);
        s.va = base + rva; s.size = vsize;
        s.bytes.assign(vsize, 0);
        uint32_t take = rawsz < vsize ? rawsz : vsize;
        if (rawoff + (size_t)take <= f.size()) memcpy(&s.bytes[0], &f[rawoff], take);
        g_secs.push_back(s);
    }
    g_loaded = g_secs.size() == 3;
    if (!g_loaded) tb_log("exe: expected .text/.rdata/.data in %s", path.c_str());
    return g_loaded;
}

const void *tb_exe_ptr(uint32_t va, size_t len) {
    if (!g_loaded && !tb_exe_load()) return nullptr;
    if (va < kEncHi && va + len > kEncLo) { tb_log("exe: 0x%x is in the encrypted CRT pages", va); return nullptr; }
    for (size_t i = 0; i < g_secs.size(); i++) {
        const Section &s = g_secs[i];
        if (va >= s.va && va + len <= s.va + s.size) return &s.bytes[va - s.va];
    }
    tb_log("exe: 0x%x (+%zu) is not in a plain section", va, len);
    return nullptr;
}

uint32_t tb_exe_u32(uint32_t va) {
    const uint8_t *p = (const uint8_t *)tb_exe_ptr(va, 4);
    return p ? (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24 : 0;
}
int32_t tb_exe_i32(uint32_t va) { return (int32_t)tb_exe_u32(va); }

const char *tb_exe_str(uint32_t va) {
    const char *p = (const char *)tb_exe_ptr(va, 1);
    if (!p) return "";
    for (size_t i = 0; i < g_secs.size(); i++) {   // make sure the string ends inside its section
        const Section &s = g_secs[i];
        if (va >= s.va && va < s.va + s.size)
            return memchr(p, 0, s.va + s.size - va) ? p : "";
    }
    return "";
}
