CC=gcc
CFLAGS=-Wall -O2

YIS=../y64sim

INSFILES = halt.sim nop.sim rrmovq.sim cmovle.sim cmovl.sim cmove.sim cmovne.sim cmovge.sim cmovg.sim irmovq.sim rmmovq.sim mrmovq.sim addq.sim subq.sim andq.sim xorq.sim jmp.sim jle.sim jl.sim je.sim jne.sim jge.sim jg.sim call.sim ret.sim pushq.sim popq.sim byte.sim word.sim long.sim quad.sim pos.sim align.sim

all: sim

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .bin .sim
.bin.sim:
	$(YIS) $*.bin > $*.sim

sim: $(INSFILES)

clean:
	rm -f *.sim *.base *~
