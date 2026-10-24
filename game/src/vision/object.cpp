#include "object.h"
#include "engine.h"
#include <string.h>
#include <ctype.h>

namespace Gorilla {

Object::Object() {}                                          // 0x424570
Object::Object(const char *n) { setName(n); }                // 0x424710
Object::~Object() {}                                         // 0x4249e0

void Object::setName(const char *n) {                        // 0x424400: trims " \f\n\r\t\v"
    std::string s = n ? n : "";
    const char *ws = " \f\n\r\t\v";
    size_t b = s.find_first_not_of(ws), e = s.find_last_not_of(ws);
    name_ = b == std::string::npos ? std::string() : s.substr(b, e - b + 1);
}

Node::~Node() {
    if (!Vision::tb_exiting()) remove();
}

static Node *link_new(Node *self, Node *after, Object *o, bool owns) {
    Node *n = new Node;
    n->obj = o;
    n->owns = owns;
    if (owns && o) o->ref();
    if (after) {                                             // insert after 'after' (0x4255d0 moves it in the array)
        n->parent = after->parent;
        n->prev = after;
        n->next = after->next;
        if (after->next) after->next->prev = n; else n->parent->last = n;
        after->next = n;
        Node *p = n->parent;
        p->kids.insert(p->kids.begin() + after->index + 1, n);
        for (size_t i = 0; i < p->kids.size(); i++) p->kids[i]->index = (int)i;
        return n;
    }
    n->parent = self;
    if (!self->last) self->first = n;
    else { n->prev = self->last; self->last->next = n; }
    self->last = n;
    n->index = (int)self->kids.size();
    self->kids.push_back(n);
    return n;
}

Node *Node::add(Node *after, Object *o) { return link_new(this, after, o, false); }       // 0x424d10
Node *Node::addOwned(Node *after, Object *o) { return link_new(this, after, o, true); }   // 0x424b70

void Node::remove() {                                        // 0x4252e0
    if (Vision::tb_exiting()) return;
    if (parent) {
        if (next) next->prev = prev; else parent->last = prev;
        if (prev) prev->next = next; else parent->first = next;
        std::vector<Node *> &k = parent->kids;
        if (index >= 0 && index < (int)k.size() && k[index] == this) k.erase(k.begin() + index);
        for (size_t i = 0; i < k.size(); i++) k[i]->index = (int)i;
        parent = nullptr; prev = nullptr; next = nullptr;
    }
    clear();
    if (owns && obj) { Object *o = obj; obj = nullptr; o->unref(); }
}

void Node::clear() {                                         // 0x425060
    while (!kids.empty()) {
        Node *k = kids[0];
        k->remove();
        delete k;
    }
    first = last = nullptr;
}

static bool ieq(const char *a, const char *b) {
    for (; *a || *b; a++, b++) if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
    return true;
}

Node *Node::find(const char *nm, int n) {                    // 0x424ea0
    const char *sl = strchr(nm, '\\');
    std::string part = sl ? std::string(nm, sl) : std::string(nm);
    for (Node *c = first; c; c = c->next) {
        if (!c->obj) continue;
        bool hit = n ? strncasecmp(part.c_str(), c->obj->name(), n) == 0 : ieq(c->obj->name(), part.c_str());
        if (hit) return sl ? c->find(sl + 1, 0) : c;
    }
    return nullptr;
}

void Node::relink() {                                        // 0x425570
    Node *p = nullptr;
    for (size_t i = 0; i < kids.size(); i++) {
        Node *k = kids[i];
        if (p) p->next = k;
        k->prev = p; k->next = nullptr; k->index = (int)i;
        p = k;
    }
    first = kids.empty() ? nullptr : kids.front();
    last = kids.empty() ? nullptr : kids.back();
}

void tb_unlink_and_delete(Node *&n) {
    if (!n) return;
    n->remove();
    delete n;
    n = nullptr;
}

}  // namespace Gorilla
