// RayTraceCharacter, reconstructed from "Tinker Bell.exe" (see character.h).
#include "character.h"

namespace Raytrace {

RayTraceCharacter::RayTraceCharacter(RayTrace *r, const char *n) : rt(r), name(n) {   // 0x4cc5c0
    x = 0.1f; refresh();
    y = 0.1f; refresh();
    z = 0.1f; refresh();
}

RayTraceCharacter::~RayTraceCharacter() {            // 0x4cc710
    RayTraceCharacter::release();
}

void RayTraceCharacter::release() {                  // 0x4cc750
    Sprite3D *s = rt->findSprite(name.c_str());
    if (s && has_box) rt->removeBox(s);
    rt->removeSprite(name.c_str());
}

Sprite3D *RayTraceCharacter::setAnim(const char *prefix, Vision::Ras *pack, int mode, int loop) {   // 0x4cc780
    Sprite3D *s = rt->findSprite(name.c_str());
    if (s) {
        if (pack) Vision::ras_set_current(pack);
        s->setAnim(prefix);
        s->mode = mode;
        s->anim->mode = loop;
        s->place();
        s->y = y + s->y;
        return s;
    }
    if (pack) Vision::ras_set_current(pack);
    rt->addSprite(name.c_str(), prefix, hit);
    s = rt->findSprite(name.c_str());
    s->load();
    s->x = x;
    s->place();
    s->y = y + s->y;
    s->z = z;
    if (has_box) {
        s->bheight = box_h;
        s->radius = box_r;
        rt->makeBox(s, true);
    }
    s->mode = mode;
    s->anim->mode = loop;
    s->f90 = 1;
    return s;
}

void RayTraceCharacter::noBox() {                    // 0x4cc880
    if (!has_box) return;
    has_box = false;
    if (Sprite3D *s = rt->findSprite(name.c_str())) rt->removeBox(s);
}

Sprite3D *RayTraceCharacter::sprite() { return rt->findSprite(name.c_str()); }   // 0x4cc8b0

void RayTraceCharacter::refresh() {                  // 0x4cc9e0 (and the end of each setter)
    Sprite3D *s = rt->findSprite(name.c_str());
    if (!s) return;
    s->x = x;
    s->place();
    s->y = y + s->y;
    s->z = z;
}
void RayTraceCharacter::setX(float v) { x = v; refresh(); }   // 0x4cc8c0
void RayTraceCharacter::setY(float v) { y = v; refresh(); }   // 0x4cc910
void RayTraceCharacter::setZ(float v) { z = v; refresh(); }   // 0x4cc960

void RayTraceCharacter::hide() {                     // 0x4cca40
    Sprite3D *s = rt->findSprite(name.c_str());
    if (s && s->anim) s->anim->visible = false;
    visible = false;
}
void RayTraceCharacter::show() {                     // 0x4cca70
    Sprite3D *s = rt->findSprite(name.c_str());
    if (s && s->anim) s->anim->visible = true;
    visible = true;
}

}  // namespace Raytrace
