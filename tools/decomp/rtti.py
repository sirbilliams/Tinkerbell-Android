#!/usr/bin/env python3
"""Recover the class map of an MSVC 6 executable from RTTI: classes, base classes, vtables and
virtual-method addresses. Imported (OMT2.dll) methods show up as thunks into the import table.

  python3 rtti.py Neutron2.exe classes.json
"""
import json, struct, sys
import pefile

pe = pefile.PE(sys.argv[1])
base = pe.OPTIONAL_HEADER.ImageBase
img = pe.get_memory_mapped_image()
def u32(va): return struct.unpack_from('<I', img, va - base)[0]
def cstr(va):
    o = va - base; e = img.index(b'\0', o); return img[o:e].decode('latin1')

secs = {s.Name.rstrip(b'\0').decode(): (base + s.VirtualAddress, base + s.VirtualAddress + s.Misc_VirtualSize) for s in pe.sections}
text = secs['.text']
def in_text(va): return text[0] <= va < text[1]
def in_img(va): return base <= va < base + len(img)

# imports: IAT slot -> name
iat = {}
for e in pe.DIRECTORY_ENTRY_IMPORT:
    for i in e.imports:
        iat[i.address] = (e.dll.decode(), i.name.decode() if i.name else f'ord{i.ordinal}')

def thunk_target(va):
    """jmp dword [iat]  (FF 25 xx xx xx xx)"""
    o = va - base
    if img[o:o + 2] == b'\xff\x25':
        slot = struct.unpack_from('<I', img, o + 2)[0]
        return iat.get(slot)
    return None

# Type descriptors: ".?AV<name>@@" preceded by vftable ptr + spare
tds = {}
pos = 0
while True:
    i = img.find(b'.?AV', pos)
    if i < 0: break
    pos = i + 4
    va = base + i - 8
    name = cstr(base + i)
    tds[va] = name[4:-2] if name.endswith('@@') else name[4:]

# Complete object locators: {0, offset, cdOffset, pTD, pCHD}
cols = {}
for off in range(0, len(img) - 20, 4):
    sig, offset, cd, ptd, pchd = struct.unpack_from('<IIIII', img, off)
    if sig == 0 and ptd in tds and in_img(pchd):
        cols[base + off] = dict(td=ptd, offset=offset, chd=pchd)

def hierarchy(chd):
    sig, attr, n, arr = struct.unpack_from('<IIII', img, chd - base)
    out = []
    for k in range(min(n, 64)):
        bcd = u32(arr + 4 * k)
        ptd, ncont, mdisp, pdisp, vdisp, battr = struct.unpack_from('<IIiiiI', img, bcd - base)
        out.append(dict(name=tds.get(ptd, hex(ptd)), mdisp=mdisp))
    return out

# vtables: dword pointing at a COL, followed by function pointers
classes = {}
col_set = set(cols)
for off in range(0, len(img) - 8, 4):
    p = struct.unpack_from('<I', img, off)[0]
    if p not in col_set: continue
    vt = base + off + 4
    methods = []
    va = vt
    while True:
        f = u32(va)
        if not in_text(f): break
        if va != vt and (va in col_set or any(u32(va - 4) == c for c in ())): break
        t = thunk_target(f)
        methods.append(dict(addr=hex(f), imported=f'{t[0]}!{t[1]}' if t else None))
        va += 4
        if struct.unpack_from('<I', img, va - base - 0)[0] in col_set: break
    c = cols[p]
    name = tds[c['td']]
    h = hierarchy(c['chd'])
    entry = classes.setdefault(name, dict(name=name, bases=[b['name'] for b in h[1:]], vtables=[]))
    entry['vtables'].append(dict(addr=hex(vt), this_offset=c['offset'], methods=methods))

out = dict(image=sys.argv[1], classes=sorted(classes.values(), key=lambda c: c['name'].lower()))
json.dump(out, open(sys.argv[2], 'w'), indent=1)
own = [c for c in out['classes'] if not c['name'].startswith(('OMedia', 'std', 'exception', 'type_info', 'length_error', 'logic_error', 'out_of_range'))]
nm = sum(len(v['methods']) for c in own for v in c['vtables'])
ni = sum(1 for c in own for v in c['vtables'] for m in v['methods'] if m['imported'])
print(f"classes {len(out['classes'])} (game {len(own)}), game vtable slots {nm}, inherited-from-OMT slots {ni}")
