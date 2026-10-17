# parallel-dot -- mini-ollama with each of the three sets of operators in
# ops/, and the checks of the two sets the lab asks you to complete:
#
#   ops/scalar.c   one element at a time, on the CPU      the baseline
#   ops/simd.c     8 floats per step with AVX2, on the CPU step 1
#   ops/cuda.cu    CUDA kernels, on the GPU               step 2
#
# Step 1, gcc only:
#   make              build mini-ollama-scalar, mini-ollama-simd and
#                     ops-selftest-simd
#   make test-simd    build, then run ./ops-selftest-simd
#   make run-simd     answer prompt.txt with the model, with ops/simd.c
#   make run-scalar   the same with ops/scalar.c, for comparison
#   make run-ollama   the same file and question in ollama, on one CPU thread
# Step 2:
#   make cuda         build mini-ollama-cuda and ops-selftest-cuda (needs nvcc)
#   make test-cuda    build, then run ./ops-selftest-cuda
#   make run-cuda     answer prompt.txt with the model, with ops/cuda.cu
#   make remote       build and run both programs of step 2 on the course
#                     server: its address is SERVER=https://host:port or the
#                     environment variable GPU_SERVER, the token the
#                     environment variable GPU_TOKEN; only ops/cuda.cu is sent
#   make clean
#
# A program holds one set of operators. main.c and selftest.c are compiled
# once per set, with -DOPS=ops_scalar, ops_simd or ops_cuda, and the object
# files of set SET are kept in build/SET/. The compiler settings that
# matter:
#   ops/simd.c is built with -mavx2, so the AVX2 intrinsics are available
#     in it and nowhere else. -mfma is not given: the lecture covers AVX2,
#     and a fused multiply-add gains little in these operators.
#   ops/scalar.c is built with -fno-tree-vectorize: GCC 12 and later
#     vectorize some loops at -O2 on their own, and the scalar operators are
#     to stay one element at a time.
#   nvcc compiles ops/cuda.cu and links the programs of step 2, so the CUDA
#     runtime library comes in. ARCH is the GPU architecture nvcc generates
#     code for; native means the GPU of this machine. CUDA_HOME is the CUDA
#     installation.
#   everything else is built at -O2.

CC        = gcc
CFLAGS    = -O2 -Wall -Wextra -I.
NOVEC     = -fno-tree-vectorize
LDLIBS    := -lm
CUDA_HOME ?= /usr/local/cuda
NVCC      := $(CUDA_HOME)/bin/nvcc
ARCH      ?= native
NVFLAGS   := -O2 -arch=$(ARCH) -I.
NVLIBS    := -lm
PYTHON    ?= python3
SERVER    ?= $(GPU_SERVER)
EXE       :=

# The files with your code, and the directory the programs (and, under
# build/, the object files) are written to. All three can be given on the
# command line.
SIMD      ?= ops/simd.c
CUDA      ?= ops/cuda.cu
O         ?= .

# The model file, q4_0.gguf in this directory (README.md, section on the
# model file), and the question put to it.
MODEL     ?= q4_0.gguf
PROMPT    ?= prompt.txt

B_SCALAR  := $(O)/build/scalar
B_SIMD    := $(O)/build/simd
B_CUDA    := $(O)/build/cuda
HEADERS   := mini_ollama.h ops.h
STEP1      = $(O)/mini-ollama-scalar$(EXE) $(O)/mini-ollama-simd$(EXE) $(O)/ops-selftest-simd$(EXE)
STEP2      = $(O)/mini-ollama-cuda$(EXE) $(O)/ops-selftest-cuda$(EXE)

# On Windows the programs are NAME.exe. In step 1 -static puts the C
# runtime into each program, so they run without the compiler's DLLs on the
# PATH. In step 2 nvcc, found through the CUDA_PATH the CUDA installer sets,
# works only with the Microsoft C compiler (cl.exe) on the PATH, and the C
# files of build/cuda/ are compiled by nvcc too, which hands them to
# cl.exe: object files of gcc and of cl.exe do not link together.
ifeq ($(OS),Windows_NT)
EXE       := .exe
LDLIBS    += -static
CUDA_HOME := $(subst \,/,$(CUDA_PATH))
NVCC      := $(CUDA_HOME)/bin/nvcc
NVLIBS    :=
$(B_CUDA)/%.o: CC = "$(NVCC)"
$(B_CUDA)/%.o: CFLAGS = -O2 -I.
$(B_CUDA)/%.o: NOVEC =
endif

all: $(STEP1)

cuda: $(STEP2)

$(O)/mini-ollama-scalar$(EXE): $(B_SCALAR)/main.o $(B_SCALAR)/model.o $(B_SCALAR)/tokenizer.o $(B_SCALAR)/scalar.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(O)/mini-ollama-simd$(EXE): $(B_SIMD)/main.o $(B_SIMD)/model.o $(B_SIMD)/tokenizer.o $(B_SIMD)/simd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(O)/ops-selftest-simd$(EXE): $(B_SIMD)/selftest.o $(B_SIMD)/model.o $(B_SIMD)/tokenizer.o \
                              $(B_SIMD)/scalar.o $(B_SIMD)/simd.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(O)/mini-ollama-cuda$(EXE): $(B_CUDA)/main.o $(B_CUDA)/model.o $(B_CUDA)/tokenizer.o \
                             $(B_CUDA)/cuda.o | have-nvcc
	"$(NVCC)" -o $@ $^ $(NVLIBS)

$(O)/ops-selftest-cuda$(EXE): $(B_CUDA)/selftest.o $(B_CUDA)/model.o $(B_CUDA)/tokenizer.o \
                              $(B_CUDA)/scalar.o $(B_CUDA)/cuda.o | have-nvcc
	"$(NVCC)" -o $@ $^ $(NVLIBS)

# The object files of set SET, in $(O)/build/SET/.
$(O)/build/%/main.o: main.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -DOPS=ops_$* -c -o $@ $<

$(O)/build/%/selftest.o: selftest.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -DOPS=ops_$* -c -o $@ $<

$(O)/build/%/model.o: model.c mini_ollama.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(O)/build/%/tokenizer.o: tokenizer.c mini_ollama.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(O)/build/%/scalar.o: ops/scalar.c ops.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(NOVEC) -c -o $@ $<

$(B_SIMD)/simd.o: $(SIMD) ops.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -mavx2 -c -o $@ $<

$(B_CUDA)/cuda.o: $(CUDA) ops.h | have-nvcc
	@mkdir -p $(@D)
	"$(NVCC)" $(NVFLAGS) -c -o $@ $<

# A missing nvcc is reported once, with what to do, before anything is
# built with it.
have-nvcc:
	@test -x "$(NVCC)" || test -x "$(NVCC).exe" || { \
	    echo "nvcc not found at $(NVCC): install the CUDA Toolkit (make CUDA_HOME=/path/to/cuda when it is elsewhere)," >&2; \
	    echo "or run your kernels on the course server with make remote" >&2; exit 2; }

$(addprefix $(B_CUDA)/,main.o selftest.o model.o tokenizer.o scalar.o): | have-nvcc

test-simd: $(O)/ops-selftest-simd$(EXE)
	$(O)/ops-selftest-simd$(EXE)

test-cuda: $(O)/ops-selftest-cuda$(EXE)
	$(O)/ops-selftest-cuda$(EXE)

# A missing model file is reported before anything is built.
have-model:
	@test -r "$(MODEL)" || { \
	    echo "no model file $(MODEL): download q4_0.gguf into this directory, or move or link it here" >&2; \
	    echo "(README.md, section on the model file), or give MODEL=path/to/q4_0.gguf" >&2; exit 2; }

run-scalar: have-model $(O)/mini-ollama-scalar$(EXE)
	$(O)/mini-ollama-scalar$(EXE) $(MODEL) $(PROMPT)

run-simd: have-model $(O)/mini-ollama-simd$(EXE)
	$(O)/mini-ollama-simd$(EXE) $(MODEL) $(PROMPT)

run-cuda: have-model $(O)/mini-ollama-cuda$(EXE)
	$(O)/mini-ollama-cuda$(EXE) $(MODEL) $(PROMPT)

# The same file and the same question in ollama: the Modelfile names
# q4_0.gguf in this directory and keeps ollama on the CPU and on one thread.
# Needs a running server (ollama serve).
run-ollama:
	@test -r q4_0.gguf || { \
	    echo "no model file q4_0.gguf: the Modelfile reads it from this directory (README.md, section on the model file)" >&2; exit 2; }
	ollama create nq-q4-0 -f Modelfile
	ollama run nq-q4-0 --verbose "$$(cat $(PROMPT))"

remote:
	@test -n "$(SERVER)" || { echo "make remote: set the environment variable GPU_SERVER (and GPU_TOKEN), or give SERVER=https://host:port" >&2; exit 2; }
	$(PYTHON) tools/submit.py --server $(SERVER) $(CUDA)

clean:
	rm -rf $(STEP1) $(STEP2) $(O)/build $(O)/*.exp $(O)/*.lib

.PHONY: all cuda have-nvcc have-model test-simd test-cuda run-scalar run-simd run-cuda run-ollama remote clean
