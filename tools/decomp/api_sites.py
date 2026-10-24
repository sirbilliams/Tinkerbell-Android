#!/usr/bin/env python3
"""Inventory of the game's API call sites in "Tinker Bell.exe".

The retail exe's copy-protection wrapper routes most API calls through one dispatcher slot, so
the call sites no longer say which API they call. This tool does NOT touch the wrapper: it reads
the game's own (intact) import table and the game code around each site, and writes candidates
that are then confirmed by hand from the decompile.

  python3 api_sites.py TinkerBell.exe api_sites.json
"""
import sys, json, struct, re
import pefile, capstone

DISPATCH_SLOT = 0x64b2f8
exe, out = sys.argv[1:3]
pe = pefile.PE(exe, fast_load=True)
img = pe.get_memory_mapped_image(); B = pe.OPTIONAL_HEADER.ImageBase

iat = {}
d = 0x130580
while True:
    oft, _, _, name, ft = struct.unpack_from('<5I', img, d)
    if not ft: break
    dll = img[name:name + 32].split(b'\0')[0].decode()
    k = 0
    while True:
        t = struct.unpack_from('<I', img, oft + 4 * k)[0]
        if not t: break
        nm = ('ord%d' % (t & 0xffff)) if t & 0x80000000 else img[t + 2:t + 64].split(b'\0')[0].decode('latin1')
        iat[B + ft + 4 * k] = (dll, nm)
        k += 1
    d += 20

text = img[0x1000:0x113000]
direct = {}
for m in re.finditer(rb'(\xff[\x15\x25]|\x8b[\x05\x0d\x15\x1d\x2d\x35\x3d]|\xa1)(....)', text, re.S):
    v = struct.unpack('<I', m.group(2))[0]
    if v in iat: direct.setdefault(iat[v][1], []).append(hex(B + 0x1000 + m.start()))

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
sites = []
for m in re.finditer(re.escape(b'\xff\x15' + struct.pack('<I', DISPATCH_SLOT)), text):
    a = B + 0x1000 + m.start()
    # the 40 bytes before the call, decoded from several start points, keep the decode that lands on the call
    best = []
    for back in range(40, 0, -1):
        ins = list(md.disasm(img[a - B - back:a - B + 6], a - back))
        if ins and ins[-1].address == a: best = ins[:-1]; break
    pushes = [f'{i.mnemonic} {i.op_str}' for i in best[-8:]]
    strs = []
    for i in best[-8:]:
        for v in re.findall(r'0x[0-9a-f]{6}', i.op_str):
            v = int(v, 16)
            if B <= v < B + len(img):
                s = img[v - B:v - B + 64].split(b'\0')[0]
                if len(s) >= 3 and all(32 <= c < 127 for c in s): strs.append(s.decode())
    after = [f'{i.mnemonic} {i.op_str}' for i in list(md.disasm(img[a - B + 6:a - B + 30], a + 6))[:3]]
    sites.append(dict(site=hex(a), before=pushes, after=after, strings=strs))

never = sorted(f'{d}!{n}' for d, n in iat.values() if n not in direct)
json.dump(dict(imports=len(iat), direct=direct, not_direct=never, sites=sites), open(out, 'w'), indent=1)
print(len(iat), 'imports;', len(direct), 'referenced directly;', len(never), 'only via the dispatcher;', len(sites), 'sites')
