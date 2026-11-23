# mini-cpu: an x86-64 machine built over five labs.
#
#   make              build mini-cpu and mini-unit (and mini-jit from lab 4 on)
#                     from src/
#   make LAB=n        build as of lab n instead of the highest lab present
#   make guests       rebuild the guest programs
#   make check        run tests/lab1 .. tests/labN
#   make handin       pack src/lab1 .. src/labN into labN.zip for submission
#   make clean
#
# LAB defaults to the highest src/labN present. The update that publishes a
# lab adds src/labN and replaces the framework, so pulling it into the
# directory of the previous lab raises LAB by one without touching the
# student's earlier code.
#
# SRCDIR is the directory holding lab1 .. lab5, BINDIR the directory the two
# programs are written to, and RT the host operators linked from lab 4 on;
# all default to this directory's layout.

SRCDIR  ?= src
BINDIR  ?= .
RT      ?= model/rt.c
LABS    := $(sort $(patsubst $(SRCDIR)/lab%,%,$(wildcard $(SRCDIR)/lab[1-5])))
LAB     ?= $(lastword $(LABS))

CC      := gcc
CFLAGS  := -std=c11 -O2 -g -Wall -Wextra -pthread -Iinclude -Imodel -D_GNU_SOURCE -DLAB=$(LAB)
LDFLAGS := -lm -pthread
# frame/memtrace.c sees these calls first (mini-cpu --memtrace, --cache 0)
LDFLAGS += -Wl,--wrap=mmu_translate,--wrap=cache_read,--wrap=cache_write,--wrap=cache_flush

upto     = $(filter-out $(wordlist $(shell expr $(LAB) + 1),9,1 2 3 4 5 6 7 8 9),$(1))
LABDIRS  = $(foreach lab,$(call upto,1 2 3 4 5),$(1)/lab$(lab))

# Framework files: frame/cpu/ the hardware of the machine, frame/os/ the
# software that runs on it, frame/ the programs of the host around them. A
# stand-in is linked until the lab that replaces it.
CORE    := frame/cpu/x86.c frame/cpu/x86rest.c frame/cpu/arith.c frame/cpu/simd.c \
           frame/cpu/commit.c frame/cpu/mem.c frame/cpu/cr3.c frame/cpu/dram.c \
           frame/os/loader.c frame/os/sysbase.c frame/os/trap.c frame/os/frames.c \
           frame/os/copy.c frame/memtrace.c frame/translate.c frame/disasm.c
ifeq ($(filter 2 3 4 5,$(LAB)),)
CORE    += frame/cpu/nocache.c
endif
ifneq ($(filter 4 5,$(LAB)),)
CORE    += frame/os/objfile.c frame/os/arena.c frame/os/hostcall.c $(RT)
endif
ifeq ($(filter 3 4 5,$(LAB)),)
CORE    += frame/cpu/pagemap.c frame/os/pagemap.c
endif
ifneq ($(filter 5,$(LAB)),)
CORE    += frame/cpu/blkdev.c frame/os/blkdrv.c
endif

# Files of labs 1 .. LAB.
STU     := $(foreach lab,$(call upto,1 2 3 4 5),$(wildcard $(SRCDIR)/lab$(lab)/*.c))
HDR     := include/mcpu.h $(BINDIR)/.lab

BIN     := $(BINDIR)/mini-cpu $(BINDIR)/mini-unit
ifneq ($(filter 4 5,$(LAB)),)
BIN     += $(BINDIR)/mini-jit
endif

# Guest programs. Their compiler options are part of the lab:
#   -nostdlib -nostdinc -ffreestanding  no libc; mini.h makes the system calls
#   -no-pie -fno-pic                    the loader only accepts ET_EXEC
#   -mno-sse                            integer code only: no xmm register in
#                                       what the compiler emits. The vector
#                                       subset of lab 1 appears only in the
#                                       hand-written guest/linear.s
#   -Og                                 disassembly stays close to the C source
GCFLAGS := -std=c11 -Og -fno-pic -no-pie -nostdlib -nostdinc -ffreestanding \
           -mno-sse -fno-asynchronous-unwind-tables -fno-stack-protector \
           -Wall -Iguest
GUESTS  := guest/hello guest/sum guest/matmul guest/heap guest/deep guest/pattern \
           guest/simd guest/block guest/tinytf guest/readonly
ifneq ($(filter 2 3 4 5,$(LAB)),)
GUESTS  += guest/rowcol
endif
ifneq ($(filter 5,$(LAB)),)
GUESTS  += guest/cat guest/tee guest/readn
endif
FILES   := guest/poem.txt guest/numbers.txt guest/scratch.txt

# mini-ollama, a program that generates text from a quantized model, as a
# guest (guest/ollama). It computes with floats and is built with -O2; in
# the options of the other guests, these take the place of -mno-sse:
#   -msse2 -mno-avx -mfpmath=sse        floats in xmm registers, no VEX
#                                       encodings from the compiler
#   -fno-tree-vectorize                 one weight at a time
#   -fno-math-errno                     sqrtf is the instruction sqrtss
#   -fno-tree-loop-distribute-patterns  the loop of memcpy in libc.c stays a
#                                       loop and does not become a call of
#                                       memcpy
# guest/mini-ollama multiplies one weight at a time (matvec.c),
# guest/mini-ollama-simd 32 at a time (matvec_simd.c and dot_q4.s, the only
# file with vector instructions). guest/mini-ollama-ref is the second
# program with dot_q4 in C (dot_q4.c): it runs on a machine without AVX2
# and prints the same text, which make check compares with. OLLAMA_SRC is
# every source file of mini-ollama.
OLLAMA        := guest/mini-ollama guest/mini-ollama-simd guest/mini-ollama-ref
OLLAMA_SRC    := guest/ollama/mini_ollama.c guest/ollama/model.c guest/ollama/tokenizer.c \
                 guest/ollama/libc.c guest/ollama/math.c guest/ollama/matvec.c \
                 guest/ollama/matvec_simd.c guest/ollama/dot_q4.s guest/ollama/dot_q4.c \
                 guest/ollama/mini_ollama.h guest/ollama/libc.h
OLLAMA_CFLAGS := $(subst -mno-sse,-msse2 -mno-avx -mfpmath=sse -fno-tree-vectorize \
                 -fno-math-errno -fno-tree-loop-distribute-patterns,$(subst -Og,-O2,$(GCFLAGS)))
# The files all three programs are built from.
OLLAMA_BOTH   := guest/start.S $(filter-out %/matvec.c %/matvec_simd.c %/dot_q4.c %.s %.h,$(OLLAMA_SRC))

# Layers of the model (lab 4). -mno-sse keeps floating point out of guest
# code: the operators are host functions reached through a system call.
LAYERS  := model/layer0.o model/layer1.o model/layer2.o model/layer3.o model/head.o
LAYER_CFLAGS := -std=c11 -Og -fno-pic -mno-sse -fno-asynchronous-unwind-tables \
                -Wall -Imodel

all: $(BIN) guests $(if $(filter 4 5,$(LAB)),$(LAYERS)) \
     $(if $(filter 5,$(LAB)),disk.img)

$(BINDIR)/mini-cpu: $(CORE) frame/main.c $(STU) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDFLAGS)

$(BINDIR)/mini-jit: $(CORE) frame/jit.c $(STU) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDFLAGS)

# mini-unit calls single functions of the labs directly (tests/unit.c).
$(BINDIR)/mini-unit: $(CORE) tests/unit.c $(STU) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDFLAGS)

# .lab holds the lab number of the last build, so changing LAB rebuilds.
$(BINDIR)/.lab: FORCE
	@echo $(LAB) | cmp -s - $@ || echo $(LAB) > $@

guests: $(GUESTS) $(OLLAMA) guest/tiny.gguf

guest/simd: guest/simd.c guest/linear.s guest/mini.h guest/start.S
	$(CC) $(GCFLAGS) -o $@ guest/start.S guest/simd.c guest/linear.s

guest/tinytf: guest/tinytf.c guest/linear.s guest/mini.h guest/start.S
	$(CC) $(GCFLAGS) -o $@ guest/start.S guest/tinytf.c guest/linear.s

guest/%: guest/%.c guest/mini.h guest/start.S
	$(CC) $(GCFLAGS) -o $@ guest/start.S $<

guest/%: guest/%.s
	$(CC) $(GCFLAGS) -o $@ $<

guest/mini-ollama: $(OLLAMA_SRC) guest/mini.h guest/start.S
	$(CC) $(OLLAMA_CFLAGS) -o $@ $(OLLAMA_BOTH) guest/ollama/matvec.c

guest/mini-ollama-simd: $(OLLAMA_SRC) guest/mini.h guest/start.S
	$(CC) $(OLLAMA_CFLAGS) -o $@ $(OLLAMA_BOTH) guest/ollama/matvec_simd.c guest/ollama/dot_q4.s

guest/mini-ollama-ref: $(OLLAMA_SRC) guest/mini.h guest/start.S
	$(CC) $(OLLAMA_CFLAGS) -o $@ $(OLLAMA_BOTH) guest/ollama/matvec_simd.c guest/ollama/dot_q4.c

# A model of two layers for guest/mini-ollama, in the format of the real one.
guest/tiny.gguf: tools/mkmodel.py
	python3 tools/mkmodel.py > $@

model/layer%.o: model/layer%.c model/layer.h model/rt.h
	$(CC) $(LAYER_CFLAGS) -c -o $@ $<

# The head is assembly: it uses the vector instructions of lab 1.
model/head.o: model/head.S
	$(CC) -c -o $@ $<

disk.img: $(FILES) tools/mkfs.py
	python3 tools/mkfs.py $@ $(FILES)

# The submission is src/lab1 .. src/labN, the .c and .h files only; grading
# unpacks it into a fresh copy of the framework, so changes outside src/ are
# not part of it. Later labs build on earlier ones, hence all of them.
handin:
	@test -n "$(LAB)" || { echo "handin: no $(SRCDIR)/labN directory" >&2; exit 1; }
	rm -f lab$(LAB).zip
	zip -qr lab$(LAB).zip $(call LABDIRS,$(SRCDIR)) -i '*.c' '*.h'
	unzip -Z1 lab$(LAB).zip

# tests/grade.sh runs tests/lab1 .. labN/run.sh on the programs in BINDIR
# and adds up their points.
check: all
	@sh tests/grade.sh $(BINDIR) $(LAB) $(SRCDIR)

clean:
	rm -f $(BINDIR)/mini-cpu $(BINDIR)/mini-jit $(BINDIR)/mini-unit $(BINDIR)/.lab $(GUESTS) \
	      guest/cat guest/tee guest/readn guest/rowcol $(LAYERS) disk.img dram.img \
	      $(OLLAMA) guest/tiny.gguf

.PHONY: all guests check handin clean FORCE
