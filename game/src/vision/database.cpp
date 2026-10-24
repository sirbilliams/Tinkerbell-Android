#include "database.h"
#include "tb.h"
#include <stdio.h>
#include <sys/stat.h>

namespace Vision {

Database::Database(const std::string &name, const std::string &ext, const std::string &folder) {
    // port: the exe resolves "<folder>\<name>.<ext>" against the install folder (and the CD, 0x5a68f8, when
    // reading); the port uses the game folder only, which is where the saves live.
    path_ = tb_game_path((folder + "/" + name + "." + ext).c_str());
}

static std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

bool Database::load() {                              // 0x4e4980
    sections_.clear();
    FILE *f = fopen(path_.c_str(), "rb");
    if (!f) return false;
    std::string text;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
    fclose(f);
    // port: the exe can decrypt a database when a key is set (+0x60, 0x4eb240); no database sets one.
    std::string comment;
    Section *cur = nullptr;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t e = text.find('\n', pos);
        if (e == std::string::npos) e = text.size();
        std::string line = trim(text.substr(pos, e - pos));
        pos = e + 1;
        if (line.empty()) continue;
        if (line.compare(0, 2, "//") == 0) { comment += (comment.empty() ? "" : "\r\n") + line; continue; }
        if (line[0] == '[') {
            size_t close = line.find(']');
            sections_.push_back(Section());
            cur = &sections_.back();
            cur->name = line.substr(1, close == std::string::npos ? std::string::npos : close - 1);
            cur->comment = comment;
            comment.clear();
            continue;
        }
        if (!cur) continue;                           // port: lines before the first section are dropped
        size_t eq = line.find('=');
        Entry en;
        en.key = trim(line.substr(0, eq));
        en.value = eq == std::string::npos ? std::string() : trim(line.substr(eq + 1));
        cur->entries.push_back(en);
    }
    return true;
}

bool Database::write() {                             // 0x4e4e30 via 0x4e4ca0
    std::string out;
    for (size_t i = 0; i < sections_.size(); i++) {
        const Section &s = sections_[i];
        if (!s.comment.empty()) out += s.comment + "\r\n";      // "%s\r\n"
        out += "[" + s.name + "]\r\n";                          // "[%s]\r\n"
        for (size_t k = 0; k < s.entries.size(); k++)           // 0x4e4ba0
            out += s.entries[k].key + "=" + s.entries[k].value + "\r\n";
        out += "\r\n";                                          // 0x53f33c
    }
    std::string dir = path_.substr(0, path_.rfind('/'));
    mkdir(dir.c_str(), 0755);
    FILE *f = fopen(path_.c_str(), "wb");
    if (!f) { tb_log("Database: cannot write %s", path_.c_str()); return false; }
    bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
    fclose(f);
    return ok;
}

bool Database::remove() {                            // 0x4e9860
    sections_.clear();
    return ::remove(path_.c_str()) == 0;
}

Database::Section *Database::section(const std::string &name, bool create) {   // 0x4e4fb0
    for (size_t i = 0; i < sections_.size(); i++)
        if (sections_[i].name == name) return &sections_[i];
    if (!create) return nullptr;
    sections_.push_back(Section());
    sections_.back().name = name;
    return &sections_.back();
}

const std::string *Database::get(const std::string &sec, const std::string &key) {
    Section *s = section(sec, false);
    if (!s) return nullptr;
    for (size_t i = 0; i < s->entries.size(); i++)
        if (s->entries[i].key == key) return &s->entries[i].value;   // 0x424ea0 (port: case-sensitive, unsure)
    return nullptr;
}

void Database::set(const std::string &sec, const std::string &key, const std::string &val) {
    Section *s = section(sec, true);
    for (size_t i = 0; i < s->entries.size(); i++)
        if (s->entries[i].key == key) { s->entries[i].value = val; return; }
    Entry e; e.key = key; e.value = val;
    s->entries.push_back(e);                          // 0x4e9a00 adds a missing key
}

}  // namespace Vision
