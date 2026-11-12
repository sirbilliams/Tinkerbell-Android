import sys, pefile, struct, capstone
pe = pefile.PE('/home/claude/decomp/TinkerBell.exe', fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
def rd(va, n): return pe.get_data(va - base, n)
if sys.argv[1] == 'f':
    for a in sys.argv[2:]:
        v = int(a, 16); print(a, struct.unpack('<f', rd(v, 4))[0], struct.unpack('<d', rd(v, 8))[0])
else:
    va = int(sys.argv[1], 16); n = int(sys.argv[2], 16)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for i in md.disasm(rd(va, n), va): print('%x %s %s' % (i.address, i.mnemonic, i.op_str))
