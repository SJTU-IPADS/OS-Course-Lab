#!/bin/sh
# 判定脚本。默认查学生的 nano-quant，带 -ref 时查参考实现。
#
#   sh tests/run.sh          判 ./nano-quant ./nq2gguf ./nq-selftest
#   sh tests/run.sh -ref     判 ./nano-quant-ref 等，助教自查用
#
# 每一项打一行，全部通过时退出码为 0。

set -e
S=${1:-}
Q=./nano-quant$S
G=./nq2gguf$S
T=./nq-selftest$S
F=tests/fixtures
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

fail=0
ok()   { printf '  通过  %s\n' "$1"; }
bad()  { printf '  未过  %s\n' "$1"; fail=$((fail+1)); }
check() { if [ "$2" = "$3" ]; then ok "$1"; else bad "$1（得 $2，应为 $3）"; fi; }

[ -f $F/tiny.safetensors ] || { echo "先跑 make fixtures"; exit 2; }

echo "一、块格式与位操作"
if $T --ref tests/reference-blocks.txt >"$W/self" 2>"$W/selferr"; then
    ok "nq-selftest 与参考一致"
else
    bad "nq-selftest 有项目对不上"
    grep -v 与参考一致 "$W/self" | sed 's/^/    /'
fi
sed 's/^/    /' "$W/selferr"

echo "二、safetensors 读取"
# 头长度写成大端必须被拒
if $Q plan $F/tiny-be.safetensors >/dev/null 2>"$W/be"; then
    bad "大端头长度居然被接受了"
else
    ok "大端头长度被拒"
fi
# 偏移超过 4 GiB 要能读
$Q plan $F/big.safetensors --recipe q4_0 >"$W/big" 2>/dev/null
check "4 GiB 之后的张量读得到" "$(wc -l <"$W/big" | tr -d ' ')" "2"

echo "三、清单与账目"
$Q plan $F/tiny.safetensors --recipe q4_k_m >"$W/plan" 2>/dev/null
check "选中的张量数"   "$(wc -l <"$W/plan" | tr -d ' ')" "46"
check "视觉塔被排除"   "$(grep -c visual "$W/plan" || true)" "0"
check "词表用 Q6_K"    "$(awk '$1=="token_embd.weight"{print $2}' "$W/plan")" "Q6_K"
check "第 2 层 ffn_down 用 Q6_K" "$(awk '$1=="blk.2.ffn_down.weight"{print $2}' "$W/plan")" "Q6_K"
check "第 0 层 ffn_down 用 Q4_K" "$(awk '$1=="blk.0.ffn_down.weight"{print $2}' "$W/plan")" "Q4_K"
check "字节总数"       "$(awk '{s+=$NF} END{print s}' "$W/plan")" "1637888"

echo "四、量化产物"
$Q quant $F/tiny.safetensors --recipe q4_k_m -o "$W/tiny.nq" >"$W/q" 2>/dev/null
check "数据区 md5" "$(awk '{print $1}' "$W/q")" "$(cat tests/reference-tiny-nq.md5)"
check "文件长度"   "$(stat -c%s "$W/tiny.nq")" "1640768"

echo "五、拼成 GGUF"
python3 tools/mkmeta.py $F/tiny-meta -o "$W/meta.kv" --name tiny >/dev/null
$G "$W/tiny.nq" --meta "$W/meta.kv" -o "$W/tiny.gguf" >/dev/null 2>&1
if python3 tools/ggufdump.py "$W/tiny.gguf" >"$W/dump"; then
    ok "GGUF 排布自洽"
else
    bad "GGUF 排布有问题"
    sed 's/^/    /' "$W/dump"
fi

echo
if [ $fail -eq 0 ]; then
    echo "全部通过"
else
    echo "$fail 项未过"
fi
exit $fail
