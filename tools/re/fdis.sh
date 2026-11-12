#!/bin/sh
# fdis.sh <hexaddr> : disassemble a whole function (size from functions.json) without nops
a=$1
sz=$(python3 -c "import json;f=json.load(open('/home/claude/decomp/raw/functions.json'));print(hex(f['0x$a']['size'] if '0x$a' in f else f[str(int('$a',16))]['size']))")
python3 /home/claude/tools/tbdis.py $a ${sz#0x} | grep -v " nop$"
