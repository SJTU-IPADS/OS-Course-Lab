CC=gcc
CFLAGS=-Wall -O2

YIS=../y64sim

APPFILES = abs-asum-cmov.sim abs-asum-jmp.sim asum.sim asumr.sim cjr.sim j-cc.sim poptest.sim pushquestion.sim pushtest.sim prog1.sim prog2.sim prog3.sim prog4.sim prog5.sim prog6.sim prog7.sim prog8.sim prog9.sim prog10.sim ret-hazard.sim

all: sim

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .bin .sim
.bin.sim:
	$(YIS) $*.bin > $*.sim

sim: $(APPFILES)

clean:
	rm -f *.sim *.base *~
