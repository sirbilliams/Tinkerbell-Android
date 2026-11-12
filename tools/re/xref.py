# xref.py imm <hexvalue>  : instructions using the 32-bit value (a global, a vtable, a string) as an operand
# xref.py disp <hexoffset> : instructions using it as a displacement ([reg + off]: a field offset)
import sys, struct, pefile, capstone
pe = pefile.PE('/home/claude/decomp/TinkerBell.exe')
b = pe.OPTIONAL_HEADER.ImageBase
t = [s for s in pe.sections if s.Name.startswith(b'.text')][0]
d, va = t.get_data(), b + t.VirtualAddress
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
v = int(sys.argv[2], 16); pat = struct.pack('<I', v); seen = set()
i = d.find(pat)
while i != -1:
    for back in range(1, 8):
        st = i - back
        for ins in md.disasm(d[st:st + 16], va + st):
            if hex(v) in ins.op_str and ins.address not in seen and ins.address + ins.size > va + i:
                seen.add(ins.address); print(hex(ins.address), ins.mnemonic, ins.op_str)
            break
    i = d.find(pat, i + 1)
