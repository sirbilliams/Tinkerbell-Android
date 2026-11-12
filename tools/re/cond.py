import sys,re
# condense: collapse "push str; lea ecx,[esp+..]; call 423f50; ... call 443260; ... call 4243a0" into vec.push(str)
lines=[l.rstrip() for l in open(sys.argv[1]) if l.strip()]
out=[];pend=[]
skip=re.compile(r'fs:\[0\]|mov byte ptr \[esp \+ 0x[0-9a-f]+\], (0x[0-9a-f]+|[a-z]+)$|mov dword ptr \[esp \+ 0x[67][0-9a-f]\], (ebx|ebp|0x[0-9a-f]+|-1)$|call 0x4243a0|lea ecx, \[esp \+ 0x1c\]$|lea (e[a-d]x), \[esp \+ 0x1c\]$|push (ecx|edx|eax)$|push 1$')
for l in lines:
    if skip.search(l): continue
    out.append(l)
print('\n'.join(out))
