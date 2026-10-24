#include "voiceover.h"
#include "../../../platform/tb.h"
#include <stdio.h>
#include <string.h>

namespace Vision {

void VoKeyClient::notify2(Service *s) {                   // 0x4c9890
    if (vo->sound && s == &vo->kbd && g_key.down && g_key.vk == 0x20 && vo->spaceStops) vo->stop();
}

VoiceOver::VoiceOver()                                    // 0x4c8ca0
    : Graphic(0, 0, 0, 0, 0, 0, true, 0xff000000u, 0, 0, 0, -1, 0xffff00ffu),
      screen(0, 0, 0, 0, 0, true, true), client(this) {
    loadLips();                                           // 0x4c8850
    kbd.setClient(&client);                               // 0x41f290
    // "lips" (+0x258): a name the exe keeps; unused by anything reconstructed so far
}

VoiceOver::~VoiceOver() {                                 // 0x4c8f70
    if (sound) { sound->stop(); sound->close(); delete sound; sound = nullptr; }
}

VoiceOver &VoiceOver::get() {
    static VoiceOver *vo = new VoiceOver();
    return *vo;
}

// 0x4c8850: MagPies.dat is "[<voice line>]" followed by one line of letters; each letter - 'a' is the mouth shape.
// (The exe reads it through its text-file reader and builds a std::map<string, vector<char>> at +0x240.)
void VoiceOver::loadLips() {
    FILE *f = fopen(tb_game_path("MagPies.dat").c_str(), "rb");
    if (!f) { vision_log("MagPies.dat: not found"); return; }
    std::string name;
    std::vector<int8_t> *cur = nullptr;
    char buf[8192];
    while (fgets(buf, sizeof buf, f)) {
        size_t n = strlen(buf);
        while (n && (buf[n - 1] == '\r' || buf[n - 1] == '\n' || buf[n - 1] == ' ')) buf[--n] = 0;
        if (!n || (buf[0] == '/' && buf[1] == '/')) continue;
        if (buf[0] == '[') {
            char *e = strchr(buf, ']');
            if (e) *e = 0;
            name = buf + 1;
            cur = &lips_[name];
            cur->clear();
            continue;
        }
        if (!cur) continue;
        for (size_t i = 0; i < n; i++) cur->push_back((int8_t)(buf[i] - 0x61));
    }
    fclose(f);
}

void VoiceOver::registerLine(const char *name, MagpieCharacter *c) { lines_[name] = c; }   // 0x4c91b0

bool VoiceOver::isPlaying(const char *name) const {       // 0x4c9690
    return cur == name && playingFlag;
}

void VoiceOver::play(const char *name) {                  // 0x4c9400
    if (!name) name = "";
    if (playingFlag && cur == name) return;
    stop();
    cur = name;
    playingFlag = true;
    sound = new Sound(cur.c_str());
    sound->mode = 2;                                      // streamed from the file
    sound->open();
    auto ch = lines_.find(cur);
    if (ch == lines_.end()) return;                       // not registered: speaker/lipsCur keep their (reset) values
    speaker = cur;
    auto lp = lips_.find(cur);
    if (lp == lips_.end()) speaker.clear();               // no mouth data: nobody animates (the exe copies 0x6311f8, empty)
    else lipsCur = &lp->second;                           // +0x250
}

void VoiceOver::stop() {                                  // 0x4c9260
    if (!sound) return;
    sound->stop();
    sound->close();
    delete sound;
    sound = nullptr;
    if (!speaker.empty()) {
        auto it = lines_.find(speaker);
        if (it != lines_.end() && it->second) it->second->rest();    // vtable +0xc
    }
    speaker.clear();
    playingFlag = false;
    startTick = 0;
    // a line that ends in "_y" is followed by the "_z" line, except two of them
    if (cur.find("_y") == std::string::npos && cur.find("_Y") == std::string::npos) return;
    if (cur == "PIRT_VO_RACawww_y") return;
    if (cur == "FLYH_VO_RAC1more_y" && skipChainFlyhMore && skipChainFlyhMore()) return;
    if (cur.empty()) { play(""); return; }
    cur[cur.size() - 1] = 'z';
    std::string next = cur;
    play(next.c_str());
}

void VoiceOver::draw() {                                  // 0x4c96d0 (Graphic::draw, once per frame)
    if (!sound) return;
    if (playingFlag && !sound->isPlaying() && startTick == 0) {
        if (!speaker.empty()) {
            MagpieCharacter *c = lines_[speaker];
            if (c && !c->ready()) return;                 // wait until the character can speak
        }
        sound->play();
        startTick = real_time();
    }
    if (sound->isPlaying()) {
        if (speaker.empty()) return;
        uint32_t idx = (real_time() - startTick) / 100;
        const std::vector<int8_t> *v = lipsCur;
        if (v && !v->empty() && idx < v->size()) {
            int c = (*v)[idx];
            if (c != -1) {
                MagpieCharacter *ch = lines_[speaker];
                if (ch) ch->mouth(c);                     // vtable +8
                return;
            }
        }
    }
    stop();
}

void VoiceOver::load() {                                  // 0x4c97f0
    kbd.activate();
    screen.layer(0)->add(this);
    screen.activate();
    client.activate();
}

void VoiceOver::unload() {                                // 0x4c9840
    Graphic::unload();
    stop();
    if (client.active()) {
        kbd.deactivate();
        screen.deactivate();
        client.deactivate();
    }
}

}  // namespace Vision
