YAS=../y64asm

all: 

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .ys .bin .yo
.ys.bin:
	$(YAS) $*.ys
.ys.yo:
	$(YAS) -v $*.ys > $*.yo

clean:
	rm -f *.yo *.bin *.base *~  


