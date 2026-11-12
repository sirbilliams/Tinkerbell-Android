#!/usr/bin/env python3
# adis.py <hexaddr> [size]: disassemble a function, annotating string/float immediates, call targets (with the
# names the port gives them, from "// 0x......" comments) and jump tables are left raw.
import sys, re, json, struct, glob, pefile, capstone
pe = pefile.PE('/home/claude/decomp/TinkerBell.exe', fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
img = pe.get_memory_mapped_image()
fns = json.load(open('/home/claude/decomp/raw/functions.json'))
port = {}
for f in glob.glob('/home/claude/tinkerbell-android/game/src/**/*.*', recursive=True):
    for l in open(f, errors='ignore'):
        for m in re.finditer(r'0x(4[0-9a-f]{5})\b', l):
            a = int(m.group(1), 16)
            if a not in port:
                s = l.strip()
                mm = re.search(r'([A-Za-z_][\w:~]*)\s*\(', s)
                port[a] = (mm.group(1) if mm else s[:40])
def s_at(a):
    o = a - base
    if o < 0 or o >= len(img): return None
    b = img[o:o + 80]
    n = b.find(b'\0')
    if n >= 3 and all(32 <= c < 127 for c in b[:n]): return b[:n].decode()
    return None
def name(a):
    k = hex(a)
    n = fns[k]['name'] if k in fns else None
    p = port.get(a)
    return ' '.join(x for x in [n, '[' + p + ']' if p else None] if x)
a = int(sys.argv[1], 16)
n = int(sys.argv[2], 16) if len(sys.argv) > 2 else fns[hex(a)]['size']
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
for i in md.disasm(img[a - base:a - base + n], a):
    if i.mnemonic == 'nop': continue
    ann = []
    if i.mnemonic == 'call' and i.op_str.startswith('0x'):
        ann.append(name(int(i.op_str, 16)))
    for m in re.finditer(r'0x([0-9a-f]{6,8})', i.op_str):
        v = int(m.group(1), 16)
        if i.mnemonic == 'call' or i.mnemonic.startswith('j'): continue
        s = s_at(v)
        if s: ann.append('"%s"' % s)
        elif 0x500000 <= v < 0x660000 and ('dword ptr [' in i.op_str or 'qword ptr [' in i.op_str) and i.mnemonic.startswith('f'):
            o = v - base
            if 'qword' in i.op_str: ann.append('%g' % struct.unpack('<d', img[o:o + 8])[0])
            else: ann.append('%g' % struct.unpack('<f', img[o:o + 4])[0])
        elif i.mnemonic in ('push', 'mov') and 0x30000000 <= v < 0x48000000 and not s:
            ann.append('f=%g' % struct.unpack('<f', struct.pack('<I', v))[0])
    print('%x %s %s%s' % (i.address, i.mnemonic, i.op_str, ('    ; ' + ' | '.join(ann)) if ann else ''))
