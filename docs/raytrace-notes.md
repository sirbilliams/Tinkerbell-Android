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

## Ported so far (game/src/raytrace/raytrace.*)
- Seg (0x48), seg_init 0x427560, Column (0x30), cast 0x425f60 (+ fallback 0x4279a0), subdivide 0x426480,
  Angle table 0x427130 (6*w entries = 60 degree view; [a, cos, sin, cos*4096, sin*4096]).
- x87 precision: MSVC6 runs at 53 bits: values that stay on the FPU stack are doubles, stores to float fields
  round. Follow the disassembly for every ftol (0x4f3948, truncation).

## Findings
- .rtc (walls): record header 0x58; u32 ntex; ntex x 40-byte texture names (0x426f00 finds each sp5 via ras_find;
  +0x4bc[i] = mirrored flag); u32 ground texture (+0x4c0 -> +0x4d0 = its pixels, mask/shift fields
  +0x4ec..+0x50c from its width); u32 sky texture (+0x518); then wall list A (+0x534) and list B (+0x538):
  records {i32 tex (-1 ends, -2 invisible), f32 x1, z1, x2, z2, i32 base} (x1.. read into locals: order
  [0x20],[0x24],[0x28],[0x1c] = x1, z1, x2, z2 per the 0x427560 call). Per textured wall a "wall object"
  (0x42e5d0, 0xb4 bytes, name "wall_<tex>_<NN>", +0x578 count, +0x57c array) with its end points as floats
  (+0x8c,+0x94 / +0x98,+0xa0) and height * 2.47 (+0x9c) - these are depth-sorted with the sprites.
  Wall angle +0x20 = atan2(...) - pi*0.5.
- Draw order (end of 0x426710): objects (+0x1148, 8-byte {obj, dist} pairs, qsort) far to near; between them
  0x4265c0(far, near) draws the wall columns in that distance band; 0x42e120(obj) draws the object; a last
  0x4265c0 for the nearest band.
- Columns: cast at 0 and w-1 for lists A and B, then subdivide(0, w-1, hitA0, hitB0, hitAlast, hitBlast).
- Wall objects (0x42e5d0 ctor(name, anim prefix); 0xb4 bytes): +0x3c name, +0x64 animation prefix, +0x30 its
  Animation (0x404c20), +0x8c..+0xa0 end points/height, +0xa4 ?, +0xa8 a key (drawn in order of it, 1e9 - key),
  +0xac frame time, +0xb0 animated flag, +0x34 last frame time, +4..+0x28 up to ten linked wall objects.
  0x4265c0(far, near): for each wall object whose distance (+0x968 pairs) is in the band, draw it (0x42d000) and
  its linked ones in key order.
- 0x42d000 draws a wall object as projected quads: steps the view in 2*0x419170() slices, projects the two end
  points (0x42df80), clips (0x42f9f0), fills a trapezoid (0x41eae0) with the current animation frame. Constants
  0x513dc8, 0x513d90, 0x513da4, 0x513dc0 (double) still to read. So the engine is more than a column ray caster:
  the column cast (0x425f60) gives depth/visibility per column; textured walls are projected quads.
- Sky 0x42a140 and ground 0x42c460 -> mode 0x42bf50 (speed rating >= 4; 0x4190c0 benchmark) are ported. Ground
  table scale = (160 + eye) * 590 * 16; ground 16.16 coordinates, 128 x 128 grid of 256-texel tiles (.grn).
- TrapezoidRenderer (0x4319a0, draw 0x431cc0) ported: vertical trapezoid, image stored sideways (row = strip,
  texel 0 at the bottom), 18.14 fixed point; corners = c * scale * 0.01 + pos + half canvas.
- project 0x42df80(x, h, z, k, *col, *row): col = view - ftol(n * atan2(dz,dx) / 2pi) - 320 wrapped to +-n/2;
  row = ftol((1/cos(col angle)) * (1/dist) * ((160+eye)*590 - 24000 * (k*0.01) * h * 0.01)); 0x5a95b8 = 2pi
  (float, static init 0x42cff0). Constants: 0x513dd8 1e-12 (zero cos), 0x513dd0 1e-11 (zero dist).
- Wall objects ported (walls.cpp): loader part (name "wall_<tex>_<count+1>", Animation(<tex>) loaded once in loop
  mode, ends truncated to ints, top = frame width * 2.47), draw 0x42d000 (2 * rating slices; ends' angles from
  the slice middle in degrees; an end past a slice edge is cut with Line::intersect 0x42f9f0 against that edge's
  ray, the frame's strips skipped by the cut fraction), band 0x4265c0 (+ linked walls by key), distances + qsort
  (comparator 0x42cf00, far first), camera setter 0x42a570 (x/eye/z nudged off whole units by 0.01; angle =
  (deg + 30) * pi/180; view = the left edge's index; +0x570 = the centre angle).
- Sprites ported (sprites.cpp): .rts loader 0x4aa570 (header 0x58, count, name[40], anim[40] minus trailing
  digits, 13 u32: mode, x, y, z, scale, +0x8c, radius, height, frame ms, +0x90..+0x9c), Sprite3D 0x42e990 /
  load 0x42ef20, boxes 0x429380 (list +0xa8), draw 0x42e120 (scale = 24000/dist*k/cos, image centred at
  (col, row) from the canvas centre), attachments sorted by z 0x42f760. Draw order: sprites past maxdist, then
  band(sprite dist, previous) + sprite, then band(0, last). Still to do: sprite frame stepping (not in 0x426710;
  probably the scene's task), the +0xa0 box refresh 0x4294b0.

## Movement and the scene (decoded Oct 24)
- Collision (collision.cpp): boundary = list A walked by 0x427a80, each wall moved 150 in (0x427760), collinear
  ones merged, cut at neighbours (0x427b70; join 0x427f00). canMove 0x42b580: odd crossings of the ray
  (x,z)->(x,z+2*range) with the boundary = inside; boxes at eye-117.07; else +0x560 = the crossed segment's
  angle. Slide: along that angle +-90 deg, the side nearer the view (back: nearer the reverse), test 0x42b910.
  Barriers +0x1158 {Line, y0, y1} 0x42d700. range 0x4272e0 = max corner distance + 1.
- Scene physics v29 0x4a9760: dt = clamp(now - last, 20..200 ms) * 0.001; speed>=0 ? forward(dt*speed) :
  back(-dt*speed); vspeed>=0 ? raise : lower (landed -> vspeed 0 when the drop was < a quarter and > 1);
  right turns -dt*rate, left +dt*rate; friction (flying +0x16bc else +0x16b8) when neither fwd nor back
  (|speed| < max*0.1 -> 0); fwd +accel, back -brake, clamp +-max; no up/down -> vspeed 0; up +, down -,
  minus gravity, clamp +-vmax. Fields +0x168c speed, +0x1690 max 400, +0x1694 vspeed, +0x1698 vmax 200,
  +0x169c turn 40, +0x16a4 accel 400, +0x16a8 brake 400, +0x16ac up 700, +0x16b0 down 500, +0x16b4 gravity 100,
  +0x16b8 friction 8, +0x16bc 6.
- RayTrace::draw 0x426710: sky writes +0x51c itself (the draw's own +0x51c sum is dead); g_sky_top =
  240 - ftol(eye)/8, >= 0; backdrop +0x116c scrolled by (+0x1178 - view - 1 + n) * bw / n, drawn twice.
