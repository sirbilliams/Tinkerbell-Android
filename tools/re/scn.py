import sys,re
names={'0x631750':'Mermaid','0x631700':'Indian_Village','0x631710':'starry','0x631760':'lost','0x6316e0':'skull','0x631740':'coconut','0x631720':'birdspy','0x631730':'mudpie','0x6316f0':'firefly'}
games={'0x5434fc':'BirdSpy','0x543500':'ColorCloud','0x543504':'CloudBurst','0x543508':'BackwardsGo','0x54350c':'PearlToss','0x543510':'Shell','0x543514':'ShadowBox','0x543518':'EchoEcho','0x54351c':'Firefly','0x543520':'MudPie','0x543524':'Coconuts','0x543528':'FlyingHoops','0x54352c':'TotemTrail','0x543530':'PirateFlag','0x543534':'BucketToss'}
L=[l.rstrip('\n') for l in open(sys.argv[1]) if l.strip()]
out=[];i=0
while i<len(L):
    l=L[i]
    if 'mov ecx, dword ptr [0x5a7394]' in l:
        # scan ahead up to 20 lines for push G (names) and 'je'
        g=None;j=i+1;tgt=None
        while j<len(L) and j<i+22:
            m=re.search(r'push (0x6317[0-9a-f]{2}|0x6316[ef]0)',L[j])
            if m: g=m.group(1)
            if re.search(r'\bje 0x',L[j]) and 'test bl, bl' in L[j-1]:
                tgt=L[j].split()[-1];break
            j+=1
        if g and tgt:
            out.append(L[i].split()[0]+f' IF scene=={names.get(g,g)} ELSE GOTO {tgt}'); i=j+1; continue
    for k,v in games.items(): l=l.replace(k,k+'<'+v+'>')
    if re.search(r'mov dword ptr \[ebp - 4\], (edi|0x[0-9a-f]+|[0-9]+)$',l) or 'call 0x4243a0' in l: i+=1; continue
    out.append(l); i+=1
print('\n'.join(out))
