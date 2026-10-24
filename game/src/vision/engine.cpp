// The Vision engine core: globals, Service, Task (+ scheduler), Scene, Stage frame, input globals.
#include "engine.h"
#include "tb.h"
#include "../../../platform/ddraw.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

namespace Vision {

// ---------------------------------------------------------------- globals
static bool g_exiting = false;      // 0x5a7638 / 0x5a9598
static uint32_t g_frame_time = 0;   // 0x5a7520
static uint32_t g_prev_frame = 0;   // 0x5a7574
static uint32_t g_time_base = 0;    // 0x5a7524
static uint32_t g_frame_delta = 0;  // 0x5a7530
static uint32_t g_frames = 0;       // 0x5a753c
static float g_fps = 0;             // 0x5a7528
static uint32_t g_inactive_at = 0;  // 0x5a7538
static bool g_active = true;        // 0x5a7534. port: starts active (no WM_ACTIVATEAPP under Xvfb / on Android)
static bool g_paused = false;       // 0x5a752c
bool g_quit = false;                // 0x5a7628
bool g_scene_switched = false;      // 0x5a7540
bool g_input_handled = false;       // 0x5a7479
bool g_movie_playing = false;       // 0x5a73bc
bool g_developer = false;           // 0x5a7579

bool tb_exiting() { return g_exiting; }                         // 0x4215b0
void tb_set_exiting() { g_exiting = true; }
uint32_t frame_time() { return g_frame_time; }                  // 0x4207b0
uint32_t real_time() { return tb_timeGetTime() - g_time_base; } // 0x4207c0
bool is_active() { return g_active; }                           // 0x420740
bool is_paused() { return g_paused; }                           // 0x420730

void set_active(bool on) {                                      // 0x420750
    g_active = on;
    if (!on) { g_inactive_at = tb_timeGetTime() - g_time_base; return; }
    if (g_inactive_at != 0) {                                   // the clock does not run while inactive
        g_time_base = tb_timeGetTime() - g_inactive_at;
        g_inactive_at = 0;
    }
}

void vision_log(const char *fmt, ...) {                         // 0x4208c0 -> 0x4215c0 OutputDebugString
    char b[2048];
    va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    tb_log("%s", b);
}

void vision_abort(const char *fmt, ...) {                       // 0x421630
    char b[2048];
    va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    if (g_quit) return;
    g_quit = true;
    tb_log("%s", b);
    tb_log(" aborting...");
    tb_MessageBoxA(nullptr, b, stage() ? stage()->name.c_str() : "", 0x10);
    tb_log("Application aborted!");
    g_exiting = true;
    // port: the original ends the process here (exit(-1)) unless running inside the frame (is_active); the port
    // leaves through the message loop so the log and screenshot are written.
}

// ---------------------------------------------------------------- Service
Node &service_kinds() { static Node n; return n; }              // 0x41f040 (0x5a7480)

ServiceKind::ServiceKind(const char *n, int prio) : Object(n), priority(prio) {   // 0x41f0c0
    service_kinds().add(nullptr, this);
    // 0x4254c0(0x41f090, 1): bubble sort ascending by priority, then relink
    std::vector<Node *> &k = service_kinds().kids;
    for (bool sorted = false; !sorted;) {
        sorted = true;
        for (size_t i = 0; i + 1 < k.size(); i++)
            if (((ServiceKind *)k[i]->obj)->priority - ((ServiceKind *)k[i + 1]->obj)->priority > 0) {
                std::swap(k[i], k[i + 1]);
                sorted = false;
            }
    }
    service_kinds().relink();
}

Service::Service(const char *n) : Object(n) {}                  // 0x41f2a0
Service::~Service() {                                           // 0x41f2f0
    if (!tb_exiting() && node) Gorilla::tb_unlink_and_delete(node);
}
Node *Service::root() { return nullptr; }                       // 0x4f2e6a: pure virtual in the exe
void Service::activate() {                                      // 0x41f240
    if (!node) {
        Node *r = root();
        node = r->add(nullptr, this);
    }
}
void Service::deactivate() {                                    // 0x41f260
    if (!tb_exiting() && node) Gorilla::tb_unlink_and_delete(node);
}

void dispatch(Node &root) {                                     // 0x41f390
    g_input_handled = true;
    std::vector<Service *> list;                                // collected from the last to the first
    for (Node *n = root.last; n; n = n->prev) list.push_back((Service *)n->obj);
    for (size_t i = 0; i < list.size(); i++) {
        Service *s = list[i];
        if (s->client) { s->client->notify(s); s->client->notify2(s); }
        else vision_log("Null client for service %s.", s->name());
        if (!g_input_handled) break;
    }
}

// ---------------------------------------------------------------- Keyboard / Mouse
static Node &keyboard_root() { static Node n; return n; }      // 0x5a6de0
static Node &mouse_root() { static Node n; return n; }         // 0x5a6e80
Node *Keyboard::root() { return &keyboard_root(); }
Node *Mouse::root() { return &mouse_root(); }
KeyEvent g_key;
MouseEvent g_mouse;

void key_event(uint32_t vk, bool up, bool down, bool chr, bool shift) {   // 0x418410
    g_key.vk = vk; g_key.up = up; g_key.down = down; g_key.chr = chr; g_key.shift = shift;
    dispatch(keyboard_root());
}
void mouse_event(int x, int y, bool up, bool down, bool move, bool shift, bool ctrl, bool lbutton, bool right) {   // 0x418620
    g_mouse.x = x; g_mouse.y = y;
    g_mouse.lup = up && !right; g_mouse.rup = up && right;
    g_mouse.ldown = down && !right; g_mouse.rdown = down && right;
    g_mouse.move = move; g_mouse.shift = shift; g_mouse.ctrl = ctrl; g_mouse.lbutton = lbutton; g_mouse.rbutton = right;
    g_input_handled = true;
    dispatch(mouse_root());
}

// ---------------------------------------------------------------- Task coroutines
// port: the original suspends a task with setjmp(task+0x48) + longjmp(0x631c60) back into the scheduler and
// resumes it by longjmp-ing into the task's (dead) frame, which only works on x86 with its stack layout.
// Here each task gets its own thread (kept for the task's life) and the scheduler and the task hand control
// to each other, so exactly one of them runs at a time. (bionic has no makecontext, so no ucontext.)
struct CoAbort {};
struct Coroutine {
    std::thread th;
    std::mutex m;
    std::condition_variable cv;
    bool task_turn = false, done = false, abort = false, orphan = false;
};
static Coroutine *g_current_co = nullptr;

static void co_switch_to_task(Coroutine *c) {
    std::unique_lock<std::mutex> l(c->m);
    c->task_turn = true;
    c->cv.notify_all();
    c->cv.wait(l, [c] { return !c->task_turn; });
}

void Task::yield() {
    Coroutine *c = co;
    if (!c || g_current_co != c) { vision_log("Task %s: wait() outside its run()", name()); return; }
    std::unique_lock<std::mutex> l(c->m);
    c->task_turn = false;
    c->cv.notify_all();
    c->cv.wait(l, [c] { return c->task_turn; });
    if (c->abort) throw CoAbort();
}

static void co_thread(Coroutine *c, Task *t) {
    for (;;) {
        {
            std::unique_lock<std::mutex> l(c->m);
            c->cv.wait(l, [c] { return c->task_turn; });
        }
        if (c->abort) break;
        try { t->run(); } catch (CoAbort &) {}
        std::unique_lock<std::mutex> l(c->m);
        c->done = true;
        c->task_turn = false;
        c->cv.notify_all();
        if (c->abort) break;
    }
    bool orphan;
    {
        std::unique_lock<std::mutex> l(c->m);
        orphan = c->orphan;
        c->task_turn = false;
        c->cv.notify_all();
    }
    if (orphan) delete c;
}

void Task::resume_or_run() {
    if (!co) {
        co = new Coroutine;
        co->th = std::thread(co_thread, co, this);
    }
    Coroutine *c = co, *prev = g_current_co;
    c->done = false;
    g_current_co = c;
    co_switch_to_task(c);
    g_current_co = prev;
}

static void co_kill(Task *t) {
    Coroutine *c = t->co;
    if (!c) return;
    t->co = nullptr;
    if (c == g_current_co) {            // a task deleting itself from inside run(): unwinds on its next yield
        c->abort = true;
        c->orphan = true;
        c->th.detach();
        return;
    }
    c->abort = true;
    co_switch_to_task(c);
    if (c->th.joinable()) c->th.join();
    delete c;
}

void Task::wait(uint32_t ms) {                                  // 0x420e50 + setjmp(+0x48) / longjmp(0x631c60)
    suspended = true;
    wake = frame_time() + ms;
    yield();
}

// ---------------------------------------------------------------- Task
static Node &task_list() { static Node n; return n; }          // 0x5a75f0 (Task::root, 0x402e20)
static Node &finished_tasks() { static Node n; return n; }     // 0x5a75c0

Task::Task(const char *n) : Service(n) {}                       // 0x421260
Task::~Task() {                                                 // 0x421150
    if (!tb_exiting()) {
        for (Node *c = children.first; c;) {
            Task *t = (Task *)c->obj;
            c = c->next;
            if (t->state == 4 || t->state == 3) t->finish();
            else if (t->state == 2) { t->state = 1; t->wake = 0; }
        }
        Service::deactivate();
        if (finished_node) Gorilla::tb_unlink_and_delete(finished_node);
    }
    co_kill(this);
}
Node *Task::root() { return &task_list(); }
void Task::activate() {                                         // 0x420e70
    Service::activate();
    if (skip) { skip = false; return; }
    for (Node *c = children.first; c; c = c->next) {
        Task *t = (Task *)c->obj;
        if (t->state == 3) t->activate();
    }
}
void Task::deactivate() {                                       // 0x420eb0
    Service::deactivate();
    if (skip) return;
    for (Node *c = children.first; c; c = c->next) {
        Task *t = (Task *)c->obj;
        if (t->state == 3) t->deactivate();
    }
}
void Task::reset() {                                            // 0x421110
    field40 = 0; suspended = false; wake = 0;
    for (Node *c = children.first; c; c = c->next) {
        Task *t = (Task *)c->obj;
        if (t->state == 3) t->reset();
    }
}
void Task::start(Task *child) {                                 // 0x420ef0
    child->state = 2;
    child->finished_node = children.add(nullptr, child);
    child->reset();
    child->parent = this;
    child->activate();
}
void Task::call(Task *child) {                                  // 0x420f60
    child->state = 4;
    skip = true;
    deactivate();
    child->finished_node = children.add(nullptr, child);
    child->reset();
    child->parent = this;
    child->activate();
}
void Task::finish() {                                           // 0x420fe0
    field40 = 0; suspended = false; wake = 0;
    for (Node *c = children.first; c; c = c->next) {
        Task *t = (Task *)c->obj;
        if (t->state == 3) t->reset();
    }
    deactivate();
    if (parent) {
        if (state == 4) parent->activate();
        parent->notify(this);
        parent->notify2(this);
        parent = nullptr;
    }
    for (Node *c = children.first; c; c = c->next) {
        Task *t = (Task *)c->obj;
        if (t->state == 2) { t->finished_node = nullptr; t->wake = 0; t->state = 1; }
    }
    if (state >= 0 && state <= 4) finished_tasks().addOwned(nullptr, this);
    if (finished_node) Gorilla::tb_unlink_and_delete(finished_node);
}

void tasks_frame() {                                            // 0x421350
    finished_tasks().clear();
    for (int i = 0;; i++) {
        Node &l = task_list();
        if (i < 0 || i >= (int)l.kids.size()) break;
        Task *t = (Task *)l.kids[i]->obj;
        if (!t) break;
        if (t->wake < frame_time()) t->resume_or_run();         // setjmp(0x631c60) == 0 -> +0x28
        if (g_quit) break;
    }
}

// ---------------------------------------------------------------- Scene
static Node &scene_list() { static Node n; return n; }         // 0x631c20
static Scene *g_pending = nullptr;                              // 0x5a7390
static Scene *g_current = nullptr;                              // 0x5a7394

Scene::Scene(const char *n) : Task(nullptr) {                   // 0x41dfa0
    setName(n);
    scene_node = scene_list().add(nullptr, this);
}
Scene::~Scene() {                                               // 0x41df30
    if (!tb_exiting() && scene_node) Gorilla::tb_unlink_and_delete(scene_node);
}
Scene *current_scene() { return g_current; }

Scene *find_scene(const char *n) {                              // 0x41de20
    for (Node *s = scene_list().first; s; s = s->next)
        if (strcasecmp(s->obj->name(), n) == 0) return (Scene *)s->obj;
    return nullptr;
}
void change_scene(const char *n) {                              // 0x41ddf0
    Scene *s = find_scene(n);
    if (s) { g_pending = s; return; }
    vision_abort("Scene %s not available.", n);
}

struct SceneKind : ServiceKind {                                // vtable 0x513978
    SceneKind() : ServiceKind("Scene", 10000) {}
    void init() override {                                      // 0x41e110
        scene_list().sort([](Object *a, Object *b) { return strcasecmp(a->name(), b->name()); });   // 0x41df10
        for (Node *n = scene_list().first; n; n = n->next) {
            Scene *s = (Scene *)n->obj;
            s->sceneInit();
            s->sceneInit2();
        }
    }
    void frame() override {                                     // 0x41e240
        if (!g_pending) {
            if (g_current) g_current->sceneTick();
            return;
        }
        if (g_current) { g_current->leave(); g_current->leave2(); }
        if (g_pending->loads == 0) { g_pending->load(); g_pending->load2(); }
        g_pending->loads++;
        g_pending->previous = g_current;
        g_current = g_pending;
        g_current->enter1();
        g_current->enter();
        g_pending = nullptr;
        g_input_handled = false;                                // 0x41f4e0
        g_scene_switched = true;
    }
    void shutdown() override {                                  // 0x41e1c0
        if (g_current) { g_current->leave(); g_current->leave2(); }
        g_current = nullptr;
        for (Node *n = scene_list().first; n; n = n->next) {
            Scene *s = (Scene *)n->obj;
            s->sceneShutdown();
            s->sceneShutdown2();
        }
    }
};

struct TaskKind : ServiceKind {                                 // vtable 0x513b04
    TaskKind() : ServiceKind("Task", 0) {}
    void frame() override { tasks_frame(); }
};

// ---------------------------------------------------------------- Stage
static Stage *g_stage = nullptr;
Stage *stage() { return g_stage; }

Stage::Stage(const char *n, const char *first, const char *cd, int w_, int h_)   // 0x421d90
    : name(n ? n : ""), first_scene(first ? first : ""), cd_path(cd ? cd : ""), w(w_), h(h_) {
    g_stage = this;
    // The rest of 0x421d90 splits cd_path ("Tinker Bell\setup\game": the volume label, then the folder on the
    // disc) for the CD search in WinMain. port: the installed game folder replaces the CD (see winmain.cpp).
}

void Stage::frame() {                                           // 0x4207d0
    g_prev_frame = g_frame_time;
    g_frame_time = tb_timeGetTime() - g_time_base;
    g_frame_delta = g_prev_frame == 0 ? 0 : g_frame_time - g_prev_frame;
    uint32_t n = ++g_frames;
    if (g_frame_delta > 999) { g_frames = 0; g_fps = (float)(n * 1000.0 / g_frame_delta); }
    Node &k = service_kinds();
    for (Node *s = k.last; s; s = s->prev) {
        ServiceKind *sk = (ServiceKind *)s->obj;
        if (g_paused) sk->framePaused(); else sk->frame();
        if (g_quit) break;
    }
    tb_Sleep(1);
}

// The kinds that live in engine.cpp; the others are in their own files. The registration order is the static
// initialiser order of the exe (Input, Keyboard, Mouse, Ras, Scene, Screen, Sound, Task, QTMovie); the list is
// kept sorted by priority, so the order only matters among equal priorities.
void register_scene_task_kinds() {
    static SceneKind scene;
    static TaskKind task;
}

}  // namespace Vision
