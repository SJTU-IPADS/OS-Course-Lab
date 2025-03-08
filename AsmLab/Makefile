CC=gcc
CFLAGS=-Wall -O2
YAS=./y64asm

all: y64asm

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .ys .bin .yo
.ys.bin: .ys
	$(YAS) $<
.ys.yo:  .ys
	$(YAS) -v $< > $@

# These are the explicit rules for making y86asm and y86emu
y64asm: y64asm.c y64asm.h
	$(CC) $(CFLAGS) $< -o $@

yat: yat.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o *.yo *.bin y64asm *~  


