// The Vision engine core (lane A): Service, Task, Scene, Stage, Screen/Layer, Graphic/Image,
// Ras/Resource/Pixelmap/Palette/Alpha, reconstructed from "Tinker Bell.exe".
// Fields keep the exe's offsets in comments (+0x..); the C++ layout is the port's own.
#pragma once
#include "object.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

struct TbSurface;

namespace Vision {

using Gorilla::Node;
using Gorilla::Object;

// ---------------------------------------------------------------- globals of the engine
bool tb_exiting();                 // 0x4215b0: 0x5a7638 (and 0x5a9598, list teardown skipped) at exit
void tb_set_exiting();
uint32_t frame_time();             // 0x4207b0: 0x5a7520, the time of the current frame (ms since start)
uint32_t real_time();              // 0x4207c0: timeGetTime() - 0x5a7524
void set_active(bool on);          // 0x420750: WM_ACTIVATEAPP; time stops while inactive
bool is_active();                  // 0x420740: 0x5a7534
bool is_paused();                  // 0x420730: 0x5a752c
void vision_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));    // 0x4208c0 (OutputDebugString)
void vision_abort(const char *fmt, ...) __attribute__((format(printf, 1, 2)));  // 0x421630
extern bool g_quit;                // 0x5a7628: leave the message loop
extern bool g_scene_switched;      // 0x5a7540: flush queued input after a scene change
extern bool g_input_handled;       // 0x5a7479
extern bool g_movie_playing;       // 0x5a73bc: QuickTime owns the window, the Screen does not draw
extern bool g_developer;           // 0x5a7579: started with a command line (Escape quits)

// ---------------------------------------------------------------- Service (vtable 0x513a68)
class Service : public Object {
public:
    explicit Service(const char *name = nullptr);    // 0x41f2a0
    ~Service() override;                             // 0x41f2f0
    virtual Node *root();                            // +0x10: the list this kind of service runs in
    virtual bool active() { return node != nullptr; }   // +0x14 0x41f230
    virtual void activate();                         // +0x18 0x41f240
    virtual void deactivate();                       // +0x1c 0x41f260
    virtual void notify(Service *) {}                // +0x20 (0x402b80): event from a client's service
    virtual void notify2(Service *) {}               // +0x24
    void setClient(Service *c) { client = c; }       // 0x41f290
    Node *node = nullptr;                            // +0x28
    Service *client = nullptr;                       // +0x2c
    int field30 = 0x1d626;                           // +0x30
};

// The registered kinds of service (0x41f0c0: name, priority; vtable 0x513a44 and the per-kind ones).
class ServiceKind : public Object {
public:
    ServiceKind(const char *name, int priority);     // 0x41f0c0
    virtual void init() {}                           // +0x10
    virtual void frame() {}                          // +0x14
    virtual void shutdown() {}                       // +0x18
    virtual void framePaused() {}                    // +0x1c (0x402e30)
    int priority;                                    // +0x24
};
Node &service_kinds();                               // 0x41f040: the registry 0x5a7480

extern Service *g_modal_client;                      // 0x5a747c (set by 0x41f370): when set, dispatch() only reaches the first service whose client it is
// Event dispatch to the active services of a list (0x41f390): client->notify, notify2.
void dispatch(Node &root);

class Keyboard : public Service {                    // vtable 0x513408: an empty shell, the events are globals
public:
    Keyboard() : Service(nullptr) {}
    Node *root() override;                           // 0x402cc0 -> 0x5a6de0
};
class Mouse : public Service {                       // vtable 0x513434
public:
    Mouse() : Service(nullptr) {}
    Node *root() override;                           // 0x402c00 -> 0x5a6e80
};
// The input globals the window procedure fills (0x418410 keys, 0x418620 mouse).
struct KeyEvent { uint32_t vk; bool up, down, chr, shift, ctrl; };          // 0x5a6d9c...
struct MouseEvent { int x, y; bool move, ldown, lup, rdown, rup, shift, ctrl, lbutton, rbutton; };   // 0x5a6e60...
extern KeyEvent g_key;
extern MouseEvent g_mouse;
void key_event(uint32_t vk, bool up, bool down, bool chr, bool shift);                    // 0x418410
void mouse_event(int x, int y, bool move, bool ldown, bool lup, bool mk_shift, bool mk_ctrl, bool mk_l, bool rdown); // 0x418620

// ---------------------------------------------------------------- Task (vtable 0x513460)
struct Coroutine;
class Task : public Service {
public:
    explicit Task(const char *name = nullptr);       // 0x421260
    ~Task() override;                                // 0x421150
    Node *root() override;                           // +0x10 0x402e20 -> the run list 0x5a75f0
    void activate() override;                        // +0x18 0x420e70
    void deactivate() override;                      // +0x1c 0x420eb0
    virtual void run() {}                            // +0x28 (v10): the task's body
    virtual void tick() {}                           // +0x2c (0x402e30)

    void wait(uint32_t ms);                          // 0x420e50 + the yield idiom (setjmp(+0x48)/longjmp(0x631c60))
    void yield();
    void start(Task *child);                         // 0x420ef0: child runs alongside (state 2)
    void call(Task *child);                          // 0x420f60: this waits until the child finishes (state 4)
    void finish();                                   // 0x420fe0
    void reset();                                    // 0x421110
    void resume_or_run();                            // the scheduler's call of +0x28 (port: coroutines)

    int state = 0;                                   // +0x34: 0 idle, 1, 2 started, 3, 4 called
    uint32_t wake = 0;                               // +0x38
    bool suspended = false;                          // +0x3c
    int field40 = 0;                                 // +0x40
    bool skip = false;                               // +0x44
    Node children;                                   // +0x88 (+0x98 first)
    Task *parent = nullptr;                          // +0xb8
    Node *finished_node = nullptr;                   // +0xbc
    Coroutine *co = nullptr;                         // port: the task's own stack (was a jmp_buf at +0x48)
};
void tasks_frame();                                  // 0x421350

// ---------------------------------------------------------------- Scene (vtable 0x51391c)
class Scene : public Task {
public:
    explicit Scene(const char *name);                // 0x41dfa0
    ~Scene() override;                               // 0x41df30
    virtual void sceneInit() {}                      // +0x2c
    virtual void sceneShutdown2() {}                 // +0x30
    virtual void load() {}                           // +0x34
    virtual void enter1() {}                         // +0x38
    virtual void leave2() {}                         // +0x3c
    virtual void sceneInit2() {}                     // +0x40
    virtual void sceneShutdown() {}                  // +0x44
    virtual void load2() {}                          // +0x48
    virtual void enter() {}                          // +0x4c
    virtual void leave() {}                          // +0x50
    virtual void sceneTick() {}                      // +0x54
    Node *scene_node = nullptr;                      // +0xc0
    Scene *previous = nullptr;                       // +0xc4
    int loads = 0;                                   // +0xc8
};
Scene *find_scene(const char *name);                 // 0x41de20
void change_scene(const char *name);                 // 0x41ddf0
Scene *current_scene();                              // 0x5a7394

// ---------------------------------------------------------------- Screen / Layer (vtables 0x5139a0, 0x513a08)
class Graphic;
class Screen;
class Layer : public Object {
public:
    explicit Layer(Screen *s);
    ~Layer() override;                               // 0x41ea10
    void add(Graphic *g);                            // 0x41e930
    void remove(Graphic *g);
    void sort();                                     // 0x41e9e0
    Screen *screen;                                  // +0x24
    Node graphics;                                   // +0x28 (+0x38 first)
    bool visible = true;                             // +0x58
};

class Screen : public Service {
public:
    Screen(int x, int y, int depth, int w, int h, bool visible, bool flag79);   // 0x41eea0
    ~Screen() override;                              // 0x41ee10
    Node *root() override;                           // 0x41f020 -> 0x5a6618
    void activate() override;                        // 0x41edb0: sorted by depth
    virtual void draw();                             // +0x28 0x41eaf0
    virtual void dirty() {}                          // +0x2c
    Layer *layer(int n);                             // 0x41ece0
    void loadAll();                                  // 0x41ec40: every graphic's load (+0x3c)
    void unloadAll();                                // 0x41ec00: every graphic's unload (+0x40)
    void clear();                                    // 0x41ebc0
    Node layers;                                     // +0x34 (+0x44 first)
    std::vector<Layer *> layer_list;                 // +0x58 array of layers
    int x, y, depth, w, h;                           // +0x64 +0x68 +0x6c +0x70 +0x74
    bool visible, flag79;                            // +0x78 +0x79
};

// The canvas the graphics draw on (0x5a73a0, with 0x5a73e8 pointing at the current one).
struct Canvas {
    TbSurface *surface = nullptr;
    uint16_t *pixels = nullptr;                      // +4
    int ox = 0, oy = 0;                              // +8 +0xc
    int w = 0, h = 0;                                // +0x10 +0x14
    int pitch = 0;                                   // +0x18 (pixels)
};
Canvas *current_canvas();
void lock_canvas();                                  // 0x40e390
void unlock_canvas();                                // 0x40e620

// ---------------------------------------------------------------- Resources
class Ras;
class Resource : public Object {                     // vtable 0x5138a4
public:
    explicit Resource(const char *name);             // 0x4194a0
    ~Resource() override;                            // 0x419450
    virtual const char *ext() = 0;                   // +0x14 (factory's type name)
    virtual void parse(const uint8_t *data) {}       // +0x18
    uint8_t *setSize(uint32_t n);                    // 0x419390
    void release();                                  // 0x4193c0
    Ras *ras = nullptr;                              // +0x24
    std::vector<uint8_t> data;                       // +0x28 / +0x2c: the whole record (0x58-byte header first)
    uint32_t size() const { return (uint32_t)data.size(); }
    uint32_t u32(uint32_t ofs) const;
};

class Pixelmap : public Resource {                   // vtable 0x51373c
public:
    explicit Pixelmap(const char *n) : Resource(n) {}
    const char *ext() override { return "sp5"; }
    void parse(const uint8_t *) override;            // 0x416d70
    int x = 0, y = 0, z = 0, w = 0, h = 0, f44 = 0, bpp = 0, kind = 0;   // +0x30..+0x4c (record 0x58..0x74)
    uint32_t pixels = 0;                             // +0x50: offset of the pixel data in data
    std::string alpha_name, palette_name;            // +0x54 / +0x60
    bool converted = false;                          // record +0x90 (0x40d960)
};
class Palette : public Resource {                    // vtable 0x51371c
public:
    explicit Palette(const char *n) : Resource(n) {}
    const char *ext() override { return "plt"; }
    void parse(const uint8_t *) override;            // 0x416980: entries turned into RGB565 in place
    uint32_t count = 0;                              // +0x30
    uint32_t entries = 0;                            // +0x34: offset in data
};
class Alpha : public Pixelmap {                      // vtable 0x5137fc
public:
    explicit Alpha(const char *n) : Pixelmap(n) {}
    const char *ext() override { return "alf"; }
    void parse(const uint8_t *) override;            // 0x416e50
    Pixelmap *owner = nullptr;                       // +0x6c
};

class Device;
class Ras : public Object {                          // vtable 0x5138c4: one .wad (or a loose file)
public:
    Ras(const char *name, const char *sub, Device *dev);   // 0x41af00
    ~Ras() override;
    void load();                                     // 0x41aa60
    Resource *get(const char *name, const char *ext);      // 0x41a290
    void add(Resource *r);                           // 0x41a500
    Device *device;                                  // +0x24
    bool loner = false;                              // +0x28 / +0x6c
    int users = 0;                                   // +0x2c
    bool loaded = false;                             // +0x30
    std::string sub;                                 // +0x34
    std::map<std::string, Resource *> table;         // +0x60
    Node *list_node = nullptr;                       // +0x4c
};
class Device : public Object {                       // Ras::Device (vtable 0x5138fc): a folder ("data")
public:
    explicit Device(const char *dir);                // 0x41c470
    std::string dir;                                 // +0x24
    Node rases;                                      // +0x68..
};
Ras *ras_open(const char *name, const char *sub, Device *dev);   // 0x419b90
Resource *ras_find(const char *name, const char *ext);           // 0x41a3d0
void ras_set_current(Ras *r);                                    // 0x41a920
Device &data_device();                                           // 0x5a70b8
std::string &cd_root();                                          // 0x5a7568: "<cd>:\<dir>\" (port: the game folder)

// ---------------------------------------------------------------- Graphic / Image (vtables 0x513340, 0x51375c)
class Graphic : public Object {
public:
    Graphic(int x, int y, int z, int w, int h, int f6c, bool visible, uint32_t color, int f78, int f80, int rot,
            int hitmode, uint32_t f8c);              // 0x411b90
    ~Graphic() override;                             // 0x402830
    virtual int canvasMode() { return 1; }           // +0x10: 1 = drawn into locked pixels, 2 = through a DC
    virtual double setScale(double percent);         // +0x14 0x412660 (port: was missing; defined in animation.cpp)
    virtual void setHit(int mode, bool f90);         // +0x18
    virtual bool hit(int x, int y);                  // +0x2c
    virtual void dirty();                            // +0x34
    virtual void draw() {}                           // +0x38
    virtual void load() {}                           // +0x3c
    virtual void unload();                           // +0x40
    int left(); int top(); int right(); int bottom();   // +0x1c..+0x28
    void detach();                                   // 0x412200
    bool inBox(int x, int y);                        // 0x411ee0
    // The hit mask (+0x54, built by setHit through 0x4122a0 from the pixels that are not the colour key f8c).
    // port: one byte per pixel instead of the exe's row runs (game/src/vision/hitmask.cpp).
    virtual bool buildHitMask(std::vector<uint8_t> &, int &, int &) { return false; }
    std::vector<uint8_t> hit_mask;
    int hit_mw = 0, hit_mh = 0;
    bool has_mask = false;
    Node *node = nullptr;                            // +0x24
    Layer *layer = nullptr;                          // +0x28
    int ox, oy, oz, ow, oh;                          // +0x30..+0x40
    double scale = 100.0;                            // +0x48
    int hitmode;                                     // +0x50
    int x, y, z, w, h;                               // +0x58..+0x68
    int f6c;                                         // +0x6c
    uint32_t color;                                  // +0x70
    uint8_t translucency = 0;                        // +0x74
    int f78, f80;                                    // +0x78 +0x80
    bool visible;                                    // +0x7c
    int rotation;                                    // +0x88
    uint32_t f8c;                                    // +0x8c
    bool f90 = false;                                // +0x90
};

class Image : public Graphic {
public:
    Image(const char *name, int x, int y, int z, int w, int h, int f6c, bool visible);   // 0x4179e0
    ~Image() override;                               // 0x417bb0
    void draw() override;                            // 0x4179b0 -> 0x40d620
    void load() override;                            // 0x416f80
    void unload() override;                          // 0x417940
    bool buildHitMask(std::vector<uint8_t> &m, int &w, int &h) override;   // port: from the sprite record
    Pixelmap *pixelmap = nullptr;                    // +0x98
    Palette *palette = nullptr;                      // +0x9c
    Alpha *alpha = nullptr;                          // +0xa0
};

// The blitter (0x40c8f0) and its parameter block (0x566514).
struct BlitParams {
    int x, y, z, w, h, rotation;                     // +4..+0x18
    const Pixelmap *pm;                              // +0x1c
    const Palette *pal;                              // +0x20
    const Alpha *alpha;                              // +0x24
    int translucency;                                // +0x28
};
void blit(const BlitParams &p, uint16_t *dst, int dst_w, int dst_h, int dst_pitch);

// ---------------------------------------------------------------- Stage (vtable 0x513b48)
class Stage {
public:
    Stage(const char *name, const char *first_scene, const char *cd_path, int w, int h);   // 0x421d90
    virtual ~Stage() {}
    virtual void frame();                            // +0x00 0x4207d0
    virtual bool ok() { return true; }               // +0x04 0x422d50
    virtual void beforeServices() {}                 // +0x08
    virtual void shutdown() {}                       // +0x0c
    virtual void afterServices() {}                  // +0x10
    virtual void aborting() {}                       // +0x14
    virtual void activated() {}                      // +0x18
    virtual void input() {}                          // +0x1c
    std::string name, first_scene, cd_path;          // +4, +0x18, +0x24
    int w, h;                                        // +0x10, +0x14
};
Stage *stage();                                      // 0x5a7554

// ---------------------------------------------------------------- the service kinds the engine registers
void register_engine_services();

}  // namespace Vision
