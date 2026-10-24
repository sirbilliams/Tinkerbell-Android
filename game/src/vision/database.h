// The game's text databases (Database@Vision, vtable 0x5178e4; the reader TextDatabaseParser@ape): INI-style
// files of [sections] with key=value lines, used for the saves (Save\Globals.txt, Save\save<N>.txt) and the
// .dat files.
//   0x4e5130 ctor, 0x4e3c80 open (cached by name), 0x4e4b60 set folder, 0x4e4980 load, 0x4e4ca0 serialise,
//   0x4e4e30 write, 0x4e3dc0 close, 0x4e4fb0 section(name), 0x424ea0 find key, 0x4e9a00 find-or-add key.
#pragma once
#include <string>
#include <vector>

namespace Vision {

class Database {
public:
    struct Entry { std::string key, value; };
    struct Section { std::string name, comment; std::vector<Entry> entries; };

    // 0x4e3c80(name, ext) + 0x4e4b60(folder) + 0x4e4980: the file is <game>/<folder>/<name>.<ext>.
    Database(const std::string &name, const std::string &ext, const std::string &folder);
    bool load();                                     // 0x4e4980: false if the file doesn't exist (empty db)
    bool write();                                    // 0x4e4e30
    bool remove();                                   // 0x4e9860: deletes the file
    Section *section(const std::string &name, bool create);   // 0x4e4fb0
    const std::string *get(const std::string &sec, const std::string &key);           // 0x4e4fb0 + 0x424ea0
    void set(const std::string &sec, const std::string &key, const std::string &val);  // 0x4e4fb0 + 0x4e9a00
    const std::string &path() const { return path_; }

private:
    std::string path_;
    std::vector<Section> sections_;
};

}  // namespace Vision
