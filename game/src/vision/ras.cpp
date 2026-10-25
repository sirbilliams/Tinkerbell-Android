// Ras (resource archives: one .wad, or a loose file), Ras::Device (a folder), Resource and its factories,
// Pixelmap / Palette / Alpha (lane A).
#include "engine.h"
#include "tb.h"
#include "../../../platform/dsound.h"   // tb_fopen_ci (case-insensitive game-folder open)
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <algorithm>

namespace Vision {

void blit_forget(const Pixelmap *pm);

static std::string g_cd_root;               // 0x5a7568 / 0x5a7570
std::string &cd_root() { return g_cd_root; }

static std::string lower(const std::string &s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++) r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

// ---------------------------------------------------------------- Resource
Resource::Resource(const char *n) : Object(n) {}           // 0x4194a0
Resource::~Resource() { blit_forget((Pixelmap *)this); }   // 0x419450
uint8_t *Resource::setSize(uint32_t n) {                   // 0x419390 (+1 for a NUL)
    data.assign((size_t)n + 1, 0);
    data.resize(n);
    return data.data();
}
uint32_t Resource::u32(uint32_t o) const {
    if (o + 4 > data.size()) return 0;
    const uint8_t *p = &data[o];
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
void Resource::release() {                                 // 0x4193c0
    if (tb_exiting()) return;
    Ras *r = ras;
    if (r && r->loner) {                                   // a loose file's Ras goes with its last user
        if (r->users > 0 && --r->users == 0) delete r;
        ras = nullptr;
        return;
    }
    unref();
}

static std::string cstr_at(const Resource *r, uint32_t ofs) {   // a name block: u32 length, then the text
    if (ofs == 0 || ofs + 4 > r->data.size()) return "";
    const char *s = (const char *)&r->data[ofs + 4];
    size_t max = r->data.size() - ofs - 4;
    return std::string(s, strnlen(s, max));
}
static std::string strip_ext(std::string s) {              // 0x423a20(4): drop ".sp5" / ".plt" / ".alf"
    if (s.size() >= 4 && s[s.size() - 4] == '.') s.resize(s.size() - 4);
    return s;
}

void Pixelmap::parse(const uint8_t *) {                    // 0x416d70
    x = (int32_t)u32(0x58); y = (int32_t)u32(0x5c); z = (int32_t)u32(0x60);
    w = (int32_t)u32(0x64); h = (int32_t)u32(0x68); f44 = (int32_t)u32(0x6c);
    bpp = (int32_t)u32(0x70); kind = (int32_t)u32(0x74);
    pixels = u32(0x80) + 4;
    palette_name = strip_ext(cstr_at(this, u32(0x78)));
    alpha_name = strip_ext(cstr_at(this, u32(0x7c)));
    // 0x40d960: 16-bit pixels are converted when the screen is not RGB565 (0x5a67f4/ec/e8). The port's screen is
    // always RGB565, so nothing to do.
}
void Palette::parse(const uint8_t *) {                     // 0x416980
    uint32_t o = u32(0x5c);
    count = u32(o);
    entries = o + 4;
    if (u32(0x6c) == 0) {                                  // not converted yet: B,G,R,0 -> RGB565 in place
        for (uint32_t i = 0; i < count && entries + i * 4 + 4 <= data.size(); i++) {
            uint32_t v = u32(entries + i * 4);
            uint16_t c = (uint16_t)(((v >> 8) & 0xf800) | ((v >> 5) & 0x7e0) | ((v >> 3) & 0x1f));
            data[entries + i * 2] = (uint8_t)c;
            data[entries + i * 2 + 1] = (uint8_t)(c >> 8);
        }
        data[0x6c] = 1;
    }
}
void Alpha::parse(const uint8_t *) {                       // 0x416e50
    x = (int32_t)u32(0x58); y = (int32_t)u32(0x5c); z = (int32_t)u32(0x60);
    w = (int32_t)u32(0x64); h = (int32_t)u32(0x68); f44 = (int32_t)u32(0x6c);
    bpp = (int32_t)u32(0x70); kind = (int32_t)u32(0x74);
    pixels = u32(0x78) + 4;
}

// The factories (Factory@Pixelmap 0x417ea0, @Palette 0x417d50, @Alpha 0x418010), keyed by the extension.
static Resource *make_resource(const std::string &ext, const char *name) {
    if (ext == "sp5") return Gorilla::gnew<Pixelmap>(name);
    if (ext == "plt") return Gorilla::gnew<Palette>(name);
    if (ext == "alf") return Gorilla::gnew<Alpha>(name);
    return nullptr;                                        // .wv6 and the 3D records belong to other classes
}

// ---------------------------------------------------------------- Device
static Device *g_devices[3];
Device::Device(const char *d) : dir(d ? d : "") {}         // 0x41c470
Device &data_device() { static Device d("data"); return d; }   // 0x5a70b8 ("data"); 0x5a7170 / 0x5a6fb0 unused here

// ---------------------------------------------------------------- Ras
static Node &ras_list() { static Node n; return n; }      // 0x5a7068 (all) / 0x5a709c (searched)
static Ras *g_current_ras = nullptr;                       // 0x5a71ec

Ras::Ras(const char *n, const char *s, Device *dev) : Object(n), device(dev), sub(s ? s : "") {   // 0x41af00
    loner = n && strchr(n, '.') != nullptr;
    list_node = ras_list().add(nullptr, this);
}
Ras::~Ras() {
    if (tb_exiting()) return;
    for (std::map<std::string, Resource *>::iterator it = table.begin(); it != table.end(); ++it) {
        it->second->ras = nullptr;
        it->second->unref();
    }
    table.clear();
    if (g_current_ras == this) g_current_ras = nullptr;
    if (list_node) Gorilla::tb_unlink_and_delete(list_node);
}

void Ras::add(Resource *r) {                               // 0x41a500
    std::string key = lower(std::string(r->name())) + "." + r->ext();
    if (table.count(key)) { vision_log("Attempting to add resource twice: %s.%s", r->name(), r->ext()); return; }
    r->ref();
    table[key] = r;
    r->ras = this;
}

Resource *Ras::get(const char *n, const char *ext) {       // 0x41a290
    std::map<std::string, Resource *>::iterator it = table.find(lower(std::string(n)) + "." + ext);
    return it == table.end() ? nullptr : it->second;
}

// 0x41aa60: read "<cd><device dir>\<sub>\<name>.wad" (or a loose "<name>.<ext>") and add every record.
void Ras::load() {
    if (loaded) return;
    std::string dir = g_cd_root + device->dir;
    if (!sub.empty()) dir += "\\" + sub;
    std::string path;
    const char *dot = strrchr(name(), '.');
    if (!dot) path = dir + "\\" + name() + ".wad";         // 0x419680(dir, name, "wad")
    else path = dir + "\\" + name();                       // "%s\%s"
    FILE *f = tb_fopen_ci(path.c_str());
    if (!f) { loaded = true; vision_log("Ras: cannot open %s", path.c_str()); return; }
    std::vector<uint8_t> all;
    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) all.insert(all.end(), buf, buf + n);
    fclose(f);
    if (!dot) {                                            // type 1: records back to back
        size_t o = 0;
        while (o + 0x58 <= all.size()) {
            uint32_t sz = all[o] | all[o + 1] << 8 | all[o + 2] << 16 | (uint32_t)all[o + 3] << 24;
            if (sz < 0x58 || o + sz > all.size()) { vision_abort("Ras::load read failure!"); break; }
            char nm[41]; memcpy(nm, &all[o + 4], 40); nm[40] = 0;
            char *e = strrchr(nm, '.');
            if (e) {
                *e = 0;
                Resource *r = make_resource(lower(e + 1), nm);
                if (r) {
                    r->ref();
                    r->setSize(sz);
                    memcpy(r->data.data(), &all[o], sz);
                    add(r);
                    r->parse(nullptr);                     // +0x18
                    r->unref();
                }
            }
            o += sz;
        }
    } else {                                               // type 4: one loose record
        std::string nm(name(), dot);
        Resource *r = make_resource(lower(dot + 1), nm.c_str());
        if (r) {
            r->ref();
            r->setSize((uint32_t)all.size());
            memcpy(r->data.data(), all.data(), all.size());
            add(r);
            r->parse(nullptr);
            r->unref();
        }
    }
    loaded = true;
}

// 0x419b90: the Ras for a name, created (and loaded) on first use; counts users.
Ras *ras_open(const char *n, const char *s, Device *dev) {
    if (!n) n = "";
    if (!s) s = "";
    Ras *found = nullptr;
    for (Node *x = ras_list().first; x && !found; x = x->next) {
        Ras *r = (Ras *)x->obj;
        if (strcasecmp(r->name(), n) == 0 && strcasecmp(r->sub.c_str(), s) == 0) found = r;
    }
    if (!found) {
        found = new Ras(n, s, dev);
        found->load();                                     // 0x4196d0: asks the device thread; port: loads now
    }
    found->users++;
    return found;
}

Ras *ras_set_current(Ras *r) { Ras *p = g_current_ras; g_current_ras = r; return p; }   // 0x41a920: returns the previous one

// 0x41a3d0: the current Ras first, then every Ras, then a loose "<name>.<ext>" in the data folder ("Loner Ras").
Resource *ras_find(const char *n, const char *ext) {
    Resource *r = nullptr;
    if (g_current_ras) r = g_current_ras->get(n, ext);
    if (!r) {
        for (Node *x = ras_list().first; x && !r; x = x->next) {
            Ras *ra = (Ras *)x->obj;
            if ((r = ra->get(n, ext))) g_current_ras = ra;
        }
    }
    if (!r) {                                              // 0x419e30
        std::string file = std::string(n) + "." + ext;
        Ras *loner = new Ras(file.c_str(), "", &data_device());
        loner->load();
        r = loner->get(n, ext);
        if (!r) { delete loner; return nullptr; }
        vision_log("Loner Ras created for %s.%s", n, ext);
        loner->users++;
    }
    r->ref();                                              // 0x424540
    return r;
}


// 0x41b050 then 0x419620, as the scenes call them on leaving: the Ras drops its records, then loses a user and goes
// with the last one (clearing the current Ras if it was this one).
void ras_close(Ras *r) {
    if (!r || tb_exiting()) return;
    for (std::map<std::string, Resource *>::iterator it = r->table.begin(); it != r->table.end(); ++it) {   // 0x41b050
        it->second->ras = nullptr;
        it->second->unref();
    }
    r->table.clear();
    r->loaded = false;
    if (r->users > 0 && --r->users == 0) {                    // 0x419620
        if (g_current_ras == r) g_current_ras = nullptr;
        delete r;
    }
}

}  // namespace Vision
