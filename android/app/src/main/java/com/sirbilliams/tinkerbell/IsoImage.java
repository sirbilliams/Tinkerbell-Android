package com.sirbilliams.tinkerbell;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Minimal ISO 9660 reader with Joliet long names (UCS-2 big endian), enough to copy a folder tree off
 * the game's CD image. Falls back to the plain ISO 9660 names when the disc has no Joliet volume.
 */
final class IsoImage {
    static final int SECTOR = 2048;

    static final class Entry {
        String name;
        boolean dir;
        long lba, size;
    }

    interface Sink {
        /** Called for each file before it is copied; return true to cancel. */
        boolean file(String path, long size);
    }

    private final FileChannel ch;
    private Entry root;
    private boolean joliet;

    IsoImage(FileChannel ch) throws IOException {
        this.ch = ch;
        byte[] s = new byte[SECTOR];
        boolean found = false;
        for (int sec = 16; sec < 64; sec++) {
            readFully(sec * (long) SECTOR, s, 0, SECTOR);
            if (s[1] != 'C' || s[2] != 'D' || s[3] != '0' || s[4] != '0' || s[5] != '1') {
                if (!found) throw new IOException("not an ISO 9660 image");
                break;
            }
            int type = s[0] & 255;
            if (type == 255) break;
            boolean j = type == 2 && s[88] == '%' && s[89] == '/' && (s[90] == '@' || s[90] == 'C' || s[90] == 'E');
            if ((type == 1 && root == null) || j) {
                Entry r = record(s, 156, j);
                if (r != null) { root = r; joliet = j; found = true; }
                if (j) break;
            }
        }
        if (root == null) throw new IOException("no volume descriptor");
    }

    static boolean looksLikeIso(FileChannel ch) {
        try {
            byte[] s = new byte[8];
            readAt(ch, 16 * (long) SECTOR, s, 0, 8);
            return s[1] == 'C' && s[2] == 'D' && s[3] == '0' && s[4] == '0' && s[5] == '1';
        } catch (IOException e) {
            return false;
        }
    }

    boolean isJoliet() { return joliet; }

    private static void readAt(FileChannel c, long pos, byte[] b, int off, int len) throws IOException {
        ByteBuffer bb = ByteBuffer.wrap(b, off, len);
        while (bb.hasRemaining()) {
            int n = c.read(bb, pos + bb.position() - off);
            if (n < 0) throw new IOException("unexpected end of the image");
        }
    }

    private void readFully(long pos, byte[] b, int off, int len) throws IOException { readAt(ch, pos, b, off, len); }

    private static long le32(byte[] b, int o) {
        return (b[o] & 255L) | (b[o + 1] & 255L) << 8 | (b[o + 2] & 255L) << 16 | (b[o + 3] & 255L) << 24;
    }

    private Entry record(byte[] b, int o, boolean j) {
        int len = b[o] & 255;
        if (len < 34) return null;
        Entry e = new Entry();
        e.lba = le32(b, o + 2);
        e.size = le32(b, o + 10);
        e.dir = (b[o + 25] & 2) != 0;
        int nl = b[o + 32] & 255;
        if (nl == 1 && (b[o + 33] == 0 || b[o + 33] == 1)) { e.name = b[o + 33] == 0 ? "." : ".."; return e; }
        try {
            String n = new String(b, o + 33, nl, j ? "UTF-16BE" : "ISO-8859-1");
            if (!e.dir) {
                int semi = n.lastIndexOf(';');
                if (semi >= 0) n = n.substring(0, semi);
                if (!j && n.endsWith(".")) n = n.substring(0, n.length() - 1);
            }
            e.name = n;
        } catch (java.io.UnsupportedEncodingException x) {
            return null;
        }
        return e;
    }

    List<Entry> list(Entry dir) throws IOException {
        List<Entry> out = new ArrayList<>();
        byte[] d = new byte[(int) dir.size];
        readFully(dir.lba * SECTOR, d, 0, d.length);
        int p = 0;
        while (p < d.length) {
            int len = d[p] & 255;
            if (len == 0) { p = (p / SECTOR + 1) * SECTOR; continue; }   // records never cross a sector
            if (p + len > d.length) break;
            Entry e = record(d, p, joliet);
            if (e != null && !".".equals(e.name) && !"..".equals(e.name)) out.add(e);
            p += len;
        }
        return out;
    }

    /** Finds a path like "setup/game" (case-insensitive), or null. */
    Entry find(String path) throws IOException {
        Entry cur = root;
        for (String part : path.split("/")) {
            if (part.isEmpty()) continue;
            Entry next = null;
            for (Entry e : list(cur))
                if (e.dir && e.name.equalsIgnoreCase(part)) { next = e; break; }
            if (next == null) return null;
            cur = next;
        }
        return cur;
    }

    /** Counts the bytes in a folder tree (for the progress bar). */
    long totalSize(Entry dir) throws IOException {
        long t = 0;
        for (Entry e : list(dir)) t += e.dir ? totalSize(e) : e.size;
        return t;
    }

    interface FileOpener {
        OutputStream open(String relPath) throws IOException;
    }

    /** Copies dir's tree through opener. progress gets the running byte count; return true to cancel. */
    interface Progress { boolean bytes(long done); }

    boolean copyTree(Entry dir, String rel, FileOpener opener, Progress progress, long[] done) throws IOException {
        byte[] buf = new byte[256 * 1024];
        for (Entry e : list(dir)) {
            String p = rel.isEmpty() ? e.name : rel + "/" + e.name;
            if (e.dir) {
                if (copyTree(e, p, opener, progress, done)) return true;
                continue;
            }
            try (OutputStream os = opener.open(p)) {
                long left = e.size, pos = e.lba * SECTOR;
                while (left > 0) {
                    int n = (int) Math.min(buf.length, left);
                    readFully(pos, buf, 0, n);
                    os.write(buf, 0, n);
                    pos += n; left -= n; done[0] += n;
                    if (progress.bytes(done[0])) return true;
                }
            }
        }
        return false;
    }

    /** True if the install folder looks right: "Tinker Bell.exe", data/ and audio/. */
    static boolean installOk(java.io.File dir) {
        boolean exe = false, data = false, audio = false;
        java.io.File[] fs = dir.listFiles();
        if (fs == null) return false;
        for (java.io.File f : fs) {
            String n = f.getName().toLowerCase(Locale.ROOT);
            if (n.equals("tinker bell.exe") && f.isFile()) exe = true;
            else if (n.equals("data") && f.isDirectory()) data = true;
            else if (n.equals("audio") && f.isDirectory()) audio = true;
        }
        return exe && data && audio;
    }
}
