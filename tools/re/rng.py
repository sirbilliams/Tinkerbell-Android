import sys
lo,hi=int(sys.argv[2],16),int(sys.argv[3],16)
for l in open(sys.argv[1]):
    try: a=int(l.split()[0],16)
    except: continue
    if lo<=a<hi: sys.stdout.write(l)
