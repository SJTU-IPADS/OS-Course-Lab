CC = gcc
CFLAGS = -fcf-protection=none

# dot_skip.c with -Og, so its if stays a conditional jump; the timing in
# dot_skip_time.c with -O2.
dot_skip: dot_skip.c dot_skip_time.c
	$(CC) -Og $(CFLAGS) -c dot_skip.c -o dot_skip.o
	$(CC) -O2 $(CFLAGS) dot_skip_time.c dot_skip.o -o dot_skip

clean:
	rm -f *.o dot_skip
