# jt.py <hexaddr> <n>: the n entries of a jump table (switch) at addr
import sys, struct, pefile
pe = pefile.PE('/home/claude/decomp/TinkerBell.exe', fast_load=True)
b = pe.OPTIONAL_HEADER.ImageBase
a, n = int(sys.argv[1], 16), int(sys.argv[2])
print([hex(struct.unpack('<I', pe.get_data(a - b + 4 * i, 4))[0]) for i in range(n)])
