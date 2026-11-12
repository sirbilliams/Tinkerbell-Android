# list function starts (after int3/ret padding) in a range, using call targets + prologue heuristics
import sys,pefile,capstone,struct
pe=pefile.PE('/home/claude/gamefiles/tb-a/Tinker Bell.exe',fast_load=True)
t=[s for s in pe.sections if s.Name.startswith(b'.text')][0]
ib=pe.OPTIONAL_HEADER.ImageBase; base=ib+t.VirtualAddress; d=t.get_data()
lo,hi=int(sys.argv[1],16),int(sys.argv[2],16)
targets=set()
for i in range(len(d)-5):
    if d[i]==0xe8:
        tg=base+i+5+struct.unpack('<i',d[i+1:i+5])[0]
        if lo<=tg<hi: targets.add(tg)
img=pe.get_memory_mapped_image()
for i in range(0,len(img)-4,4):
    v=struct.unpack('<I',img[i:i+4])[0]
    if lo<=v<hi: targets.add(v)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
prev=None
for a in sorted(targets):
    print(hex(a), (a-prev) if prev else '')
    prev=a
