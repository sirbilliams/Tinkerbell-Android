#!/usr/bin/env python3
"""Raw-decompile every game function of "Tinker Bell.exe" with r2ghidra, named from the RTTI class map.

  python3 decompile_all.py TinkerBell.exe classes.json outdir

Writes outdir/<Class>.c (methods found in that class's vtables, first owner wins) and
outdir/_free.c (everything else), plus outdir/functions.json (address -> name, class, size).

Tinker Bell differs from the Jimmy ports: the retail exe carries a copy-protection wrapper
(sections .cms_t/.cms_d). We do NOT analyse or remove it. Analysis is limited to the game's own
.text, minus the CRT pages the wrapper keeps encrypted (ENC_START..ENC_END); those pages are MSVC6
runtime code that the port replaces with its own runtime. API calls that the wrapper routes through its
dispatcher slot show up as `redirected_api` and are named from context (api_sites.py).
"""
import json, os, sys, re, struct
import pefile, r2pipe

CODE_START, CODE_END = 0x401000, 0x513000   # the game's .text
ENC_START, ENC_END = 0x4f3000, 0x4f8000     # CRT pages the wrapper keeps encrypted: never decompiled
def wanted(a): return CODE_START <= a < CODE_END and not ENC_START <= a < ENC_END
DISPATCH_SLOT = 0x64b2f8

exe, classes_json, outdir = sys.argv[1:4]
os.makedirs(outdir, exist_ok=True)
classes = json.load(open(classes_json))['classes']

# The game's original import table is intact in .rdata (descriptors at RVA 0x130580): name its IAT slots.
pe = pefile.PE(exe, fast_load=True)
img = pe.get_memory_mapped_image(); B = pe.OPTIONAL_HEADER.ImageBase
iat = {}
d = 0x130580
while True:
    oft, _, _, name, ft = struct.unpack_from('<5I', img, d)
    if not ft: break
    k = 0
    while True:
        t = struct.unpack_from('<I', img, oft + 4 * k)[0]
        if not t: break
        nm = ('ord%d' % (t & 0xffff)) if t & 0x80000000 else img[t + 2:t + 64].split(b'\0')[0].decode('latin1')
        iat[B + ft + 4 * k] = nm
        k += 1
    d += 20

owner = {}
for c in sorted(classes, key=lambda c: len(c['bases'])):
    for vi, vt in enumerate(c['vtables']):
        for si, m in enumerate(vt['methods']):
            a = int(m['addr'], 16)
            if m['imported'] or a in owner or not wanted(a): continue
            owner[a] = (c['name'], f"vt{vi}_{si}" if vi else f"v{si}")

r2 = r2pipe.open(exe, flags=['-2', '-e', 'scr.color=0', '-e', 'bin.demangle=true'])
r2.cmd(f'e anal.in=range; e anal.from={CODE_START}; e anal.to={CODE_END}')
r2.cmd('e r2ghidra.indent=4; e r2ghidra.casts=true')
for slot, nm in iat.items():
    r2.cmd(f'f imp.{nm} 4 @ {slot}')
r2.cmd(f'f redirected_api 4 @ {DISPATCH_SLOT}')
for a, (cls, slot) in owner.items():
    r2.cmd(f'af @ {a}; afn {re.sub(r"[^A-Za-z0-9_]", "_", cls)}__{slot} @ {a}')
r2.cmd('aar; aac; aap; aar')
funcs = [f for f in (r2.cmdj('aflj') or []) if wanted(f['addr'])]
print(f'{len(funcs)} functions', flush=True)

files = {}
index = {}
for i, f in enumerate(sorted(funcs, key=lambda f: f['addr'])):
    a = f['addr']
    cls = owner.get(a, (None, None))[0]
    name = f['name']
    try:
        code = r2.cmd(f'pdg @ {a}')
    except Exception as e:
        code = f'/* decompile failed: {e} */\n'
    key = re.sub(r'[^A-Za-z0-9_]', '_', cls) if cls else '_free'
    fh = files.get(key)
    if fh is None:
        fh = files[key] = open(os.path.join(outdir, key + '.c'), 'w')
    fh.write(f'\n/* ==== {hex(a)}  {name}  size {f.get("size", 0)} ==== */\n{code}\n')
    index[hex(a)] = dict(name=name, cls=cls, size=f.get('size', 0))
    if i % 200 == 0:
        print(f'{i}/{len(funcs)}', flush=True)
for fh in files.values(): fh.close()
json.dump(index, open(os.path.join(outdir, 'functions.json'), 'w'), indent=0)
print('done', flush=True)
