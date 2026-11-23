# kvcache -- mini-ollama with a KV cache, and the checks of src/kvcache.c,
# the file with the functions you write.
#
#   make              build mini-ollama and kvcache-selftest
#   make test         run kvcache-selftest, then the conversation of
#                     tests/workload.txt on a small generated model under
#                     several limits, each compared with --no-cache (needs
#                     Python 3)
#   make run          answer workload.txt with the model twice: with the KV
#                     cache, and with --no-cache
#   make clean
#
# The headers are in include/, the framework (the rest of mini-ollama) in
# frame/, your file in src/, kvcache-selftest (tests/selftest.c) and the
# workload checks in tests/, and the scripts the checks use in tools/.
#
# The compiler settings that matter: frame/ops.c is built with
# -fno-tree-vectorize, since GCC 12 and later vectorize some loops at -O2
# on their own and the operators are to stay one element at a time;
# everything else is built at -O2.

CC        = gcc
CFLAGS    = -O2 -Wall -Wextra -Iinclude
NOVEC     = -fno-tree-vectorize
LDLIBS   := -lm
PYTHON   ?= python3
EXE      :=

# The file with your code, and the directory the programs (and, under
# build/, the object files and the files of make test) are written to. Both
# can be given on the command line.
KVCACHE  ?= src/kvcache.c
O        ?= .

# make run: the model file, q4_0.gguf in this directory (README.md, section
# on the model file), the workload, the limit of the KV cache and the
# answer tokens per turn.
MODEL    ?= q4_0.gguf
WORKLOAD ?= workload.txt
LIMIT    ?= 32M
MAX_NEW  ?= 8

BUILD    := $(O)/build
HEADERS  := include/mini_ollama.h include/ops.h include/kvcache.h
PROGRAMS  = $(O)/mini-ollama$(EXE) $(O)/kvcache-selftest$(EXE)

# On Windows the programs are NAME.exe, and -static puts the C runtime into
# each program, so they run without the compiler's DLLs on the PATH.
ifeq ($(OS),Windows_NT)
EXE      := .exe
LDLIBS   += -static
endif

all: $(PROGRAMS)

$(O)/mini-ollama$(EXE): $(BUILD)/main.o $(BUILD)/model.o $(BUILD)/tokenizer.o $(BUILD)/ops.o $(BUILD)/kvcache.o $(BUILD)/kvmem.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(O)/kvcache-selftest$(EXE): $(BUILD)/selftest.o $(BUILD)/kvcache.o $(BUILD)/model.o $(BUILD)/tokenizer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/ops.o: frame/ops.c include/ops.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(NOVEC) -c -o $@ $<

$(BUILD)/kvcache.o: $(KVCACHE) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# kvcache-selftest calls the functions of src/kvcache.c directly.
$(BUILD)/selftest.o: tests/selftest.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: frame/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# The small model of make test: random weights, 3 layers, made by
# tools/mkmodel.py, the same file on every machine.
$(BUILD)/tiny.gguf: tools/mkmodel.py
	@mkdir -p $(@D)
	$(PYTHON) tools/mkmodel.py > $@.tmp && mv $@.tmp $@

test: $(PROGRAMS) $(BUILD)/tiny.gguf
	$(O)/kvcache-selftest$(EXE)
	$(PYTHON) tests/check.py $(O)/mini-ollama$(EXE) $(BUILD)/tiny.gguf tests/workload.txt $(BUILD)

# A missing model file is reported before anything is built.
have-model:
	@test -r "$(MODEL)" || { \
	    echo "no model file $(MODEL): download q4_0.gguf into this directory, or move or link it here" >&2; \
	    echo "(README.md, section on the model file), or give MODEL=path/to/q4_0.gguf" >&2; exit 2; }

RUN = $(O)/mini-ollama$(EXE) --kv-limit $(LIMIT) --max-new $(MAX_NEW)

run: have-model $(O)/mini-ollama$(EXE)
	$(RUN) $(MODEL) $(WORKLOAD)
	$(RUN) --no-cache $(MODEL) $(WORKLOAD)

clean:
	rm -rf $(PROGRAMS) $(BUILD)

.PHONY: all test have-model run clean
