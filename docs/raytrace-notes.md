# The 3D engine ("RayTrace"): reverse-engineering notes (work in progress)

Every Neverland area and London is a `Raytrace_Scene` (via `TinkerbellRaytraceScene`); the renderer is
`RayTrace`, a `Vision::Screen` subclass (vtable at RTTI "RayTrace", draw = v10 0x426710). Code regions:
0x426000-0x42c800 RayTrace core (66 funcs, 25 KB), 0x49a000-0x4a2000 Wall/Ground/Sprites displays (31 KB),
0x4a2000-0x4ab000 Trapezoid, transitions, Raytrace_Scene (37 KB), 0x4ab000 TinkerbellRaytraceScene (4 KB).
London-specific: 0x4b4f00-0x4c0000 (44 KB: London_Scene, LS*Script tasks, Peter/Wendy state machines).

## Raytrace_Scene::open(name) (v27 0x4a9ca0)
- start time -> +0xdc; clears +0xe0..+0xf4, +0x1680.
- activates three services (+0x1328, +0x12f4, +0x12c0); creates a 0x158-byte graphic (0x4a9410) at +0x124,
  adds it to layer 0 of ...; calls this->v28 (+0x70); creates a Task (vtable 0x5162c4, 0xc4 bytes, +0xc0 = scene)
  at +0x12c and activates it.
- Ras packs: "<name>_walls" (+0x130, set current), "<name>_sprites" (+0x134), "<name>_ground" (+0x138); the
  ground grid file "data\<name>.grn".
- activate(), Task::reset (0x421110), RayTrace (+0x140) activate, then
  0x4281a0(name) (RayTrace: init + walls), 0x4aa570(name) (sprites), 0x42bc60(?, "data\<name>.grn") (ground).
- +0x665 = 1; screens +0x14e0 and +0x155c activate + loadAll; +0x15d8 load; cursor set/shown; hotspot
  (0x5a6930/34) = 0; 0x42a570(+0xcc); 0x4aa880().

## RayTrace::load(name) (0x4281a0)
Clears its state; time fields +0x7c/+0x88 = now; colour key 0x5a95b4 = 0xff00ff as RGB565 (0xf81f);
+0x524 = 1; +0x4e0 = INT_MAX; z-buffer-ish buffer 0x5a95b0 = new short[w*h] (w +0x70, h +0x74),
+0x4d4 = its middle row; 0x427130(); +0x578 = 0; 0x428580(name) (the walls); column buffer +0x548 =
new [w * 0x60]; walks the wall lists +0x534 / +0x538 (circular, next at +0x34, flag at +0x30) for any
non-empty -> +0x530 = !found, +0x55c = 0x427b70(); clears the vector +0x1138..+0x113c (objects, 0x42ec10 dtor);
0x419170() < 4 (the display depth?) picks +0x4d8 = 3 else 2 when +0x4d0 or +0x514 is set, then 0x427620().

## RayTrace::draw (v10 0x426710), outline
Only when +0x78 (visible) and +0xa4 (ready). Frame counter +0x1164; frame time delta +0x80 (min 1);
0x5a95ac = the canvas pointer (0x41ead0); +0xa0 = bottom row pointer; +0x90 = h/2 (horizon).
Per object in +0x1138: if its +0xa0 flag, 0x4294b0(object, "<object name>_box").
If +0xa5 (camera moved?): recompute the view edge rays (0x427560 + 0x425f60 x4 into the column buffer +0x548,
stride 0x30), 0x426480(0, w-1, ...).
Horizon shift: +0x51c from +0x524 and the pitch; 0x5a95a8 = 240 - (x / 8) clamped >= 0. 0x42a140().
Background panorama (+0x116c, a graphic): x scrolled by the view angle (+0x1178, +0x574 over +0x52c steps),
drawn twice for wrap-around. 0x42c460() unless +0x1175; +0x1168 drawn.
Sprites: distance from the camera (+0x568, +0x56c = camera x, z) for every object in +0x1148 (sorted with
qsort 0x4f54fc), and for the +0x578 entries at +0x57c (two end points each, +0x8c..+0xa0).
