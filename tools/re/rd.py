import sys,struct,pefile
pe=pefile.PE('/home/claude/gamefiles/tb-a/Tinker Bell.exe',fast_load=True)
a=int(sys.argv[1],16); n=int(sys.argv[2]) if len(sys.argv)>2 else 9
d=pe.get_data(a-pe.OPTIONAL_HEADER.ImageBase,n*4)
print(' '.join(hex(x) for x in struct.unpack('<%dI'%n,d)))
