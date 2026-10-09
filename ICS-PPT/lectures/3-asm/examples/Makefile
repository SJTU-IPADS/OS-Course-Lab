CC = gcc
CFLAGS = -fcf-protection=none

all: dot_scalar dot_avx2

dot_scalar: dot.c bench.c
	$(CC) -O2 $(CFLAGS) -c dot.c -o dot_s.o
	$(CC) -O2 $(CFLAGS) bench.c dot_s.o -o dot_scalar

dot_avx2: dot.c bench.c
	$(CC) -O2 -mavx2 $(CFLAGS) -c dot.c -o dot_v.o
	$(CC) -O2 $(CFLAGS) bench.c dot_v.o -o dot_avx2

# GCC 12 and later vectorize the scalar kernel at -O2 on their own;
# -fno-tree-vectorize keeps it one weight at a time.
matvec_scalar: matvec_q4.c
	$(CC) -O2 -fno-tree-vectorize -fopenmp $(CFLAGS) matvec_q4.c -o matvec_scalar

matvec_avx2: matvec_q4.c
	$(CC) -O2 -mavx2 -fopenmp $(CFLAGS) matvec_q4.c -o matvec_avx2

# max.c compiled twice: -Og selects with a conditional jump (jl), -O2 with
# a conditional move (cmovge). max_time.c is compiled with -O2 both times.
max_jl: max.c max_time.c
	$(CC) -Og $(CFLAGS) -c max.c -o max_jl.o
	$(CC) -O2 $(CFLAGS) max_time.c max_jl.o -o max_jl

max_cmov: max.c max_time.c
	$(CC) -O2 $(CFLAGS) -c max.c -o max_cmov.o
	$(CC) -O2 $(CFLAGS) max_time.c max_cmov.o -o max_cmov

clean:
	rm -f *.o dot_scalar dot_avx2 matvec_scalar matvec_avx2 max_jl max_cmov
