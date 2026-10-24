#!/usr/bin/env python3
"""Inventory of the globals the game code touches: address, section, references, access sizes.
Only the game's .text is scanned (the wrapper sections and the encrypted CRT pages are skipped).

  python3 globals_inventory.py TinkerBell.exe globals.json
"""
import sys, json, collections
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_REG_INVALID

exe, out = sys.argv[1:3]
pe = pefile.PE(exe, fast_load=True); img = pe.get_memory_mapped_image(); B = pe.OPTIONAL_HEADER.ImageBase
secs = {s.Name.rstrip(b'\0').decode(): (B + s.VirtualAddress, B + s.VirtualAddress + s.Misc_VirtualSize) for s in pe.sections}
def sec(a):
    for n, (lo, hi) in secs.items():
        if lo <= a < hi and n in ('.data', '.rdata'): return n
md = Cs(CS_ARCH_X86, CS_MODE_32); md.detail = True; md.skipdata = True
refs = collections.defaultdict(lambda: dict(n=0, sizes=set(), sec=None))
for lo, hi in ((0x401000, 0x4f3000), (0x4f8000, 0x513000)):
    for ins in md.disasm(img[lo - B:hi - B], lo):
        if ins.id == 0: continue
        for op in ins.operands:
            if op.type == X86_OP_MEM and op.mem.base == X86_REG_INVALID and op.mem.index == X86_REG_INVALID:
                a = op.mem.disp & 0xffffffff; s = sec(a)
                if s: r = refs[a]; r['n'] += 1; r['sizes'].add(op.size); r['sec'] = s
res = {hex(a): dict(n=r['n'], sizes=sorted(r['sizes']), sec=r['sec']) for a, r in sorted(refs.items())}
json.dump(res, open(out, 'w'), indent=0)
c = collections.Counter(r['sec'] for r in res.values())
print(len(res), 'globals', dict(c))
