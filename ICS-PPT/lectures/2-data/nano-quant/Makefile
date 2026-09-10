# nano-quant —— 编译选项是判定的一部分，不要改。
#
# -ffp-contract=off 禁止把 a*b+c 合成一条 FMA 指令。合成之后中间结果不再
# 四舍五入到 float，量化边界上的取整会跟着变，换台机器就对不上账。
# -fno-fast-math 同理：不允许编译器假定浮点运算可交换可结合。

CXX      ?= c++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -fno-fast-math -Iinclude
LDFLAGS  :=

FRAME := src/fp16.cpp src/md5.cpp src/json.cpp src/st.cpp src/q6_k.cpp \
         src/quant.cpp src/recipe.cpp src/nqfile.cpp

STUDENT := student/student.cpp
REF     := ref/reference.cpp

BINS := nano-quant nq2gguf nq-selftest
REFBINS := nano-quant-ref nq2gguf-ref nq-selftest-ref

all: $(BINS)
ref: $(REFBINS)

nano-quant:      src/main_quant.cpp   $(FRAME) $(STUDENT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
nano-quant-ref:  src/main_quant.cpp   $(FRAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

nq2gguf:         src/main_nq2gguf.cpp $(FRAME) $(STUDENT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
nq2gguf-ref:     src/main_nq2gguf.cpp $(FRAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

nq-selftest:     src/main_selftest.cpp $(FRAME) $(STUDENT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
nq-selftest-ref: src/main_selftest.cpp $(FRAME) $(REF)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

fixtures:
	python3 tools/mkfixtures.py tests/fixtures

test: all ref fixtures
	sh tests/run.sh

clean:
	rm -f $(BINS) $(REFBINS)

.PHONY: all ref test clean fixtures
