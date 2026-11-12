import sys,pefile
pe=pefile.PE('/home/claude/gamefiles/tb-a/Tinker Bell.exe',fast_load=True)
for s in sys.argv[1:]:
    a=int(s,16); d=pe.get_data(a-pe.OPTIONAL_HEADER.ImageBase,80)
    print(hex(a), d.split(b'\0')[0].decode('latin1'))
