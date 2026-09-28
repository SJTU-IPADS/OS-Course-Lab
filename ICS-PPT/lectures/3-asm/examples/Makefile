CC = gcc
CFLAGS = -fcf-protection=none

all: dot_scalar dot_avx2

dot_scalar: dot.c bench.c
	$(CC) -O2 $(CFLAGS) -c dot.c -o dot_s.o
	$(CC) -O2 $(CFLAGS) bench.c dot_s.o -o dot_scalar

dot_avx2: dot.c bench.c
	$(CC) -O2 -mavx2 $(CFLAGS) -c dot.c -o dot_v.o
	$(CC) -O2 $(CFLAGS) bench.c dot_v.o -o dot_avx2

clean:
	rm -f *.o dot_scalar dot_avx2
