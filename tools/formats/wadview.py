import sys, struct
from PIL import Image
def records(d):
    o = 0
    while o + 4 <= len(d):
        n = struct.unpack_from('<I', d, o)[0]
        if n < 0x58: break
        yield d[o + 4:o + 44].split(b'\0')[0].decode('latin1'), d[o:o + n]; o += n
def palette(r):
    n = struct.unpack_from('<I', r, 0x74)[0]
    return [tuple(r[0x78 + 4 * i:0x7b + 4 * i])[::-1] for i in range(n)]
def sprite(r, pals):
    w, h = struct.unpack_from('<II', r, 0x64); bpp, kind = struct.unpack_from('<II', r, 0x70)
    po, ao, do = struct.unpack_from('<III', r, 0x78)
    o = do + 4; img = Image.new('RGBA', (w, h), (0, 0, 0, 0)); px = img.load()
    if (kind & ~0x10) == 3:
        pal = pals[r[po + 4:po + 44].split(b'\0')[0].decode()]
        esc = r[o]; o += 1; i = 0
        while i < w * h:
            b = r[o]
            if b == esc: v, n = r[o + 1], r[o + 2]; o += 3
            else: v, n = b, 1; o += 1
            for k in range(n):
                if i + k < w * h and v != esc: px[(i + k) % w, (i + k) // w] = pal[v] + (255,)
            i += n
    elif (kind & ~0x10) in (0, 1, 2):
        pal = pals[r[po + 4:po + 44].split(b'\0')[0].decode()] if bpp == 8 else None
        bp = 1 if bpp == 8 else 2
        def put(x, y):
            nonlocal o
            if bp == 1: px[x, y] = pal[r[o]] + (255,); o += 1
            else:
                p = struct.unpack_from('<H', r, o)[0]; o += 2
                px[x, y] = (((p >> 11) & 31) * 255 // 31, ((p >> 5) & 63) * 255 // 63, (p & 31) * 255 // 31, 255)
        def count():
            nonlocal o
            n = r[o]; o += 1
            if (kind & ~0x10) == 2 and n == 0xff: n = struct.unpack_from('<H', r, o)[0]; o += 2
            return n
        for y in range(h):
            if (kind & ~0x10) == 0:
                for x in range(w): put(x, y)
                continue
            x = 0
            while x < w:
                x += count()
                if x >= w: break
                n = count()
                for k in range(n): put(x + k, y)
                x += n
    else:
        return None, (w, h, bpp, kind)
    return img, (w, h, bpp, kind, o, len(r))
d = open(sys.argv[1], 'rb').read(); pals = {}; out = []
for name, r in records(d):
    if name.endswith('.plt'): pals[name] = palette(r)
    elif len(out) < int(sys.argv[3]):
        img, info = sprite(r, pals); print(name, info)
        if img: out.append(img)
W = sum(i.width for i in out) + 8 * len(out); H = max(i.height for i in out)
sheet = Image.new('RGBA', (W, H), (40, 60, 90, 255)); x = 0
for i in out: sheet.alpha_composite(i, (x, 0)); x += i.width + 8
sheet.save(sys.argv[2])
