# strcalls.py file [startaddr]: a listing reduced to calls and lea [esi+..]; runs of string pushes joined on one line
import sys
lines=open(sys.argv[1]).read().splitlines()
start=sys.argv[2] if len(sys.argv)>2 else None
on=start is None; buf=[]
skip=('0x423f50','0x443260','0x4243a0','0x443150','0x443640','0x4f2de4')
for l in lines:
    if not on:
        if l.startswith(start): on=True
        else: continue
    if '"' in l:
        buf.append(l.split('"')[1]); continue
    if any(s in l for s in skip): continue
    if 'call' in l or 'lea e' in l and '[esi + 0x' in l or 'mov ebp' in l or 'ret' in l.split()[1:2]:
        if buf: print('   S:', ' '.join(buf)); buf=[]
        print(l)
if buf: print('   S:', ' '.join(buf))
