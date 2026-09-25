# nano-quant -- keep these compiler flags: the checks compare bytes, and the
# flags are what make the bytes the same on every machine.
#
# -ffp-contract=off forbids fusing a*b+c into one FMA instruction. A fused
# product is not rounded to float, so a value on a rounding boundary can land
# on the other side, and the output bytes change from machine to machine.
# -fno-fast-math likewise: the compiler may not treat float arithmetic as
# commutative or associative.

CXX      ?= c++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -ffp-contract=off -fno-fast-math -Iinclude
LDFLAGS  :=
EXE      :=

# On Windows the programs are NAME.exe, and -static puts the C++ runtime into
# each program, so they run without the compiler's DLLs on the PATH. The
# define silences the Microsoft headers' warnings about strerror and fopen,
# which clang gives when it uses those headers.
ifeq ($(OS),Windows_NT)
EXE      := .exe
LDFLAGS  := -static
CXXFLAGS += -D_CRT_SECURE_NO_WARNINGS
endif

# The Python 3 command; on Windows it may be python: make test PYTHON=python
PYTHON   ?= python3
export PYTHON

FRAME := src/fp16.cpp src/md5.cpp src/json.cpp src/st.cpp src/q6_k.cpp \
         src/q4_k_fit.cpp src/quant.cpp src/recipe.cpp src/nqfile.cpp \
         src/file.cpp

IMPL := impl/nano_quant.cpp

BINS := nano-quant$(EXE) nq2gguf$(EXE) nq-selftest$(EXE)

all: $(BINS)

nano-quant$(EXE):  src/main_quant.cpp    $(FRAME) $(IMPL)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

nq2gguf$(EXE):     src/main_nq2gguf.cpp  $(FRAME) $(IMPL)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

nq-selftest$(EXE): src/main_selftest.cpp $(FRAME) $(IMPL)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

fixtures:
	$(PYTHON) tools/mkfixtures.py tests/fixtures

test: all fixtures
	sh tests/run.sh

clean:
	rm -f $(BINS)

.PHONY: all test clean fixtures
