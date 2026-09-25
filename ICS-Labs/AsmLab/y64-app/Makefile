CC=gcc
CFLAGS=-Wall -O2

ISADIR = ..
YAS=$(ISADIR)/y64asm

YOFILES = abs-asum-cmov.yo abs-asum-jmp.yo asum.yo asumr.yo cjr.yo j-cc.yo poptest.yo pushquestion.yo pushtest.yo prog1.yo prog2.yo prog3.yo prog4.yo prog5.yo prog6.yo prog7.yo prog8.yo prog9.yo prog10.yo ret-hazard.yo

BINFILES = abs-asum-cmov.bin abs-asum-jmp.bin asum.bin asumr.bin cjr.bin j-cc.bin poptest.bin pushquestion.bin pushtest.bin prog1.bin prog2.bin prog3.bin prog4.bin prog5.bin prog6.bin prog7.bin prog8.bin prog9.bin prog10.bin ret-hazard.bin

all: $(YOFILES) 

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .ys .bin .yo
.ys.bin:
	$(YAS) $*.ys
.ys.yo:
	$(YAS) -v $*.ys > $*.yo

bin: $(BINFILES)

yo: $(YOFILES)

clean:
	rm -f *.yo *~ *.bin
