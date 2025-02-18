CC=gcc
CFLAGS=-Wall -O2
YAS=./y64asm-base
YIS=./y64sim-base

APPFILES = abs-asum-cmov.sim abs-asum-jmp.sim asum.sim asumr.sim cjr.sim j-cc.sim poptest.sim pushquestion.sim pushtest.sim prog1.sim prog2.sim prog3.sim prog4.sim prog5.sim prog6.sim prog7.sim prog8.sim prog9.sim prog10.sim ret-hazard.sim

INSFILES = halt.sim nop.sim rrmovq.sim cmovle.sim cmovl.sim cmove.sim cmovne.sim cmovge.sim cmovg.sim irmovq.sim rmmovq.sim mrmovq.sim addq.sim subq.sim andq.sim xorq.sim jmp.sim jle.sim jl.sim je.sim jne.sim jge.sim jg.sim call.sim ret.sim pushq.sim popq.sim byte.sim word.sim long.sim quad.sim pos.sim align.sim

all: sim

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .ys .sim
.ys.sim:
	$(YAS) $*.ys
	$(YIS) $*.bin > $*.sim

sim: $(INSFILES) $(APPFILES)

clean:
	rm -f *.bin *.sim *.sim.base *~
