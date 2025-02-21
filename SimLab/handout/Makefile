CC=gcc
CFLAGS=-Wall -O2
LCFLAGS=-O2
YIS=./y64sim

Y64_C_SRC=y64sim.c
Y64_H_SRC=y64sim.h
Y64SIM_TARGET=y64sim

YAT_SRC=yat.c
YAT_TARGET=yat

all: $(Y64SIM_TARGET)

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .bin .sim
.bin.sim:
	$(YIS) $*.bin > $*.sim

# These are the explicit rules for making y86asm and y86emu
$(Y64SIM_TARGET):$(Y64_C_SRC) $(Y64_H_SRC)
	$(CC) $(CFLAGS) $< -o $@

$(YAT_TARGET):$(YAT_SRC)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(Y64SIM_TARGET) *.sim *~

.PHONY: all clean
