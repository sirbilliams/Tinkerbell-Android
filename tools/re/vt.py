import sys, pefile, struct
pe = pefile.PE('/home/claude/decomp/TinkerBell.exe', fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
def d(va): return struct.unpack('<I', pe.get_data(va-base,4))[0]
def rtti(vt):
    try:
        col = d(vt-4); td = d(col+12); s = pe.get_data(td+8-base, 80); return s.split(b'\0')[0].decode()
    except Exception: return '?'
n=int(sys.argv[1])
for a in sys.argv[2:]:
    va=int(a,16)
    print(a, rtti(va), ' '.join('%x'%d(va+4*i) for i in range(n)))
