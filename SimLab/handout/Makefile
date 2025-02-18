CC=gcc
CFLAGS=-Wall -O2
LCFLAGS=-O2
YIS=./y64sim

all: y64sim

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .bin .sim
.bin.sim:
	$(YIS) $*.bin > $*.sim

# These are the explicit rules for making y86asm and y86emu
y64sim:
	$(CC) $(CFLAGS) y64sim.c -o y64sim

yat:
	$(CC) $(CFLAGS) yat.c -o yat

clean:
	rm -f y64sim *.sim *~  


