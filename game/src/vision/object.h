// Gorilla::Object and the link nodes the engine keeps every list in (lane A).
//   0x424570 Object ctor, 0x4249e0 dtor, 0x424400 setName, 0x424540 ref, 0x424550 unref
//   0x424d10 Node::add(after, obj) (not owning), 0x424b70 (owning), 0x4252e0 Node::remove,
//   0x4250d0 Node::object, 0x425060 Node::clear, 0x424ea0 Node::find(name), 0x425400/0x4254c0 sort,
//   0x425570 relink after sort.
// A Node (0x30 bytes in the exe): +4 index in the parent's array, +8 object, +0xc owns, +0x10 first child,
// +0x14 last child, +0x18 prev, +0x1c next, +0x20 parent, +0x24 children array, +0x28 count, +0x2c capacity.
#pragma once
#include <stdint.h>
#include <string>
#include <vector>

namespace Gorilla {

class Object {
public:
    Object();                                   // 0x424570
    explicit Object(const char *name);          // 0x424710
    virtual ~Object();                          // 0x4249e0 (vtbl +0xc)
    const char *name() const { return name_.c_str(); }   // 0x4243f0
    void setName(const char *n);                // 0x424400
    void ref() { if (refs >= 0) refs++; }       // 0x424540
    void unref() { if (refs > 0 && --refs == 0) delete this; }   // 0x424550
    std::string name_;                          // +4
    int refs = -1;                              // +0x20: -1 = not from the heap (static/member), never deleted
};

// operator new for Objects (0x4243c0): the ctor finds the address in the "just allocated" table and starts the
// count at 0 instead of -1. port: the template marks the object instead.
template <class T, class... A> T *gnew(A... a) { T *o = new T(a...); o->refs = 0; return o; }

struct Node {
    int index = -1;                // +4
    Object *obj = nullptr;         // +8
    bool owns = false;             // +0xc
    Node *first = nullptr, *last = nullptr, *prev = nullptr, *next = nullptr, *parent = nullptr;
    std::vector<Node *> kids;      // +0x24 / +0x28 / +0x2c
    Node() {}
    ~Node();
    Node *add(Node *after, Object *o);          // 0x424d10
    Node *addOwned(Node *after, Object *o);     // 0x424b70
    void remove();                              // 0x4252e0 (unlink, free children; the caller deletes)
    void clear();                               // 0x425060
    Object *object() const { return obj; }      // 0x4250d0
    Node *find(const char *name, int n = 0);    // 0x424ea0 (case-insensitive, '\' separates levels)
    template <class Cmp> void sort(Cmp c) {     // 0x425400 quicksort by cmp, ties by index; 0x425570 relink
        std::vector<Node *> v = kids;
        for (size_t i = 1; i < v.size(); i++)
            for (size_t j = i; j > 0; j--) {
                int r = c(v[j]->obj, v[j - 1]->obj);
                if (r == 0) r = v[j]->index - v[j - 1]->index;
                if (r < 0) std::swap(v[j], v[j - 1]); else break;
            }
        kids = v;
        relink();
    }
    void relink();                              // 0x425570
};

void tb_unlink_and_delete(Node *&n);            // fcn.004252e0 + operator delete, then n = 0

}  // namespace Gorilla
