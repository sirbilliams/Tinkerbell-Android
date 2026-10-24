#!/bin/bash
# Runs the ORIGINAL "Tinker Bell.exe" under Wine, headless, and screenshots it at intervals: the reference
# for what the port should show. Nothing of the game goes into the repo.
#
#   tools/wine/run_original.sh <game copy dir> <out dir> <seconds> [interval=5] [display=:93] [iso]
#
# The game is only run the normal way, with the player's disc presented as a CD-ROM drive: the ISO is
# loop-mounted if that is allowed, else extracted (all of it, volume label kept) into <out dir>/disc and
# Wine's D: is pointed there as type cdrom (the label comes from <disc>/.windows-label, which Wine reads).
# No patching, debugging or tracing of the exe.
# Also set up in <out dir>: a 32-bit Wine prefix, ALSA "null" sound, Xvfb 640x480x16.
# Output: <out dir>/shot-<seconds>.png, <out dir>/wine.log
set -u
GAME=$(cd "$1" && pwd); OUT=$(mkdir -p "$2" && cd "$2" && pwd); T=$3; I=${4:-5}; DISP=${5:-:93}
ISO=${6:-/home/claude/gamefiles/tb/dl/Tinker.Bell.iso}
LABEL=TINKER_BELL
export WINEARCH=win32 WINEPREFIX=$OUT/wineprefix WINEDEBUG=-all DISPLAY=$DISP
export WINEDLLOVERRIDES="winemenubuilder.exe=d;mscoree=d;mshtml=d"

# --- the disc
DISC=$OUT/disc
if [ ! -e "$DISC/.tb_ready" ]; then
	mkdir -p "$DISC"
	if mount -o loop,ro "$ISO" "$DISC" 2>/dev/null; then
		touch "$OUT/.mounted"
	else
		python3 -I - "$ISO" "$DISC" <<'PY' || exit 1
import sys, os, pycdlib
iso = pycdlib.PyCdlib(); iso.open(sys.argv[1]); dest = sys.argv[2]
def walk(path):
    for ch in iso.list_children(joliet_path=path):
        n = ch.file_identifier().decode('utf-16-be')   # Joliet names
        if n in ('.', '..'): continue
        p = path.rstrip('/') + '/' + n
        t = os.path.join(dest, p.lstrip('/'))
        if ch.is_dir():
            os.makedirs(t, exist_ok=True); walk(p)
        else:
            iso.get_file_from_iso(t, joliet_path=p)
walk('/')
PY
	fi
	printf '%s' "$LABEL" > "$DISC/.windows-label"
	touch "$DISC/.tb_ready"
fi

# --- sound off: DirectSound then has no device, as with no sound card
if [ ! -f "$OUT/alsa.conf" ]; then
	cp /usr/share/alsa/alsa.conf "$OUT/alsa.conf"
	printf '\npcm.!default { type null }\n' >> "$OUT/alsa.conf"
fi
export ALSA_CONFIG_PATH=$OUT/alsa.conf

# the game sets 640x480x16: a private Xvfb of exactly that (an older one on this display is replaced)
pkill -f "Xvfb $DISP " 2>/dev/null && sleep 1
Xvfb "$DISP" -screen 0 640x480x16 >/dev/null 2>&1 &
sleep 1

if [ ! -d "$WINEPREFIX/dosdevices" ]; then
	wineboot -i >/dev/null 2>&1
	ln -sfn "$DISC" "$WINEPREFIX/dosdevices/d:"
	wine reg add 'HKLM\Software\Wine\Drives' /v 'd:' /d cdrom /f >/dev/null 2>&1
	wineserver -k; wineserver -w				# the drive type is read when Wine starts
fi

cd "$GAME" || exit 1
wine "Tinker Bell.exe" > "$OUT/wine.log" 2>&1 &
for ((t = I; t <= T; t += I)); do
	sleep "$I"
	import -display "$DISP" -window root "$OUT/shot-$t.png" 2>/dev/null
done
wineserver -k
[ -f "$OUT/.mounted" ] && umount "$DISC"
