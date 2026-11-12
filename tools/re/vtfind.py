# vtfind.py <RTTI name part>...: each matching class's vtable(s) (via its type descriptor and complete object
# locator) with the first 16 entries, and the code addresses that write the vtable (its ctors / dtors)
import sys, re, struct, pefile
pe = pefile.PE('/home/claude/decomp/TinkerBell.exe')
b = pe.OPTIONAL_HEADER.ImageBase
rd = [s for s in pe.sections if s.Name.startswith(b'.rdata')][0]
tx = [s for s in pe.sections if s.Name.startswith(b'.text')][0]
D, dva = rd.get_data(), b + rd.VirtualAddress
T, tva = tx.get_data(), b + tx.VirtualAddress
for s in pe.sections:
    d = s.get_data()
    for m in re.finditer(rb'\.\?AV[\w@]+\0', d):
        nm = m.group()[4:-1].decode()
        if not any(k in nm for k in sys.argv[1:]): continue
        td = b + s.VirtualAddress + m.start() - 8
        for i in range(0, len(D) - 20, 4):
            if struct.unpack_from('<I', D, i + 12)[0] != td or struct.unpack_from('<I', D, i)[0] != 0: continue
            col = struct.pack('<I', dva + i)
            j = D.find(col)
            while j != -1:
                vt = dva + j + 4
                ents = [hex(struct.unpack_from('<I', D, j + 4 + 4 * k)[0]) for k in range(16)]
                w = struct.pack('<I', vt); k = T.find(w); writes = []
                while k != -1: writes.append(hex(tva + k)); k = T.find(w, k + 1)
                print(nm, 'vtable', hex(vt), 'written at', ' '.join(writes))
                print('   ', ' '.join(ents))
                j = D.find(col, j + 1)
