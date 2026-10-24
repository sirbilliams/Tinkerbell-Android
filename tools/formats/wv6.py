import sys, struct, wave
STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX = [-1,-1,-1,-1,2,4,6,8]
def decode(data, n, lowfirst=True):
    p, i, out = 0, 0, []
    for b in data:
        for nib in ((b & 15, b >> 4) if lowfirst else (b >> 4, b & 15)):
            t = STEP[i] * (nib & 7); d = (t >> 3) + (t >> 2)   # game's formula (0x423170), not standard IMA
            p = max(-32768, min(32767, p - d if nib & 8 else p + d))
            i = max(0, min(88, i + IDX[nib & 7])); out.append(p)
            if len(out) >= n: return out
    return out
f = open(sys.argv[1], 'rb').read()
ch, bits, rate, pcmlen, _, doff = struct.unpack_from('<IIIIII', f, 0x58)
dlen = struct.unpack_from('<I', f, doff)[0]; data = f[doff + 4:doff + 4 + dlen]
n = pcmlen // 2
for lf in (True, False):
    s = decode(data, n, lf)
    rough = sum(abs(s[k] - s[k - 1]) for k in range(1, min(len(s), 200000))) / min(len(s), 200000)
    print('lowfirst' if lf else 'highfirst', 'samples', len(s), 'expected', n, 'mean |diff|', int(rough), 'peak', max(map(abs, s[:200000])))
s = decode(data, n, False)  # high nibble first, as the game (0x423170)
w = wave.open(sys.argv[2], 'wb'); w.setnchannels(ch); w.setsampwidth(2); w.setframerate(rate)
w.writeframes(struct.pack('<%dh' % len(s), *s)); w.close()
print('ch', ch, 'bits', bits, 'rate', rate, 'secs', round(n / rate / ch, 1))
