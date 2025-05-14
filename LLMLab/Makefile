.PHONY: all clean benchmark

CFLAGS += -O3 -g -Wall -pthread

all: gpt gpt_naive

gpt: gpt.c task.c kvcache.c
	gcc $(CFLAGS) gpt.c task.c kvcache.c -o gpt -lm -pthread

gpt_naive: gpt_naive.c
	gcc $(CFLAGS) gpt_naive.c -o gpt_naive -lm -pthread

run:
	time taskset -c 0-3 ./gpt -m gpt2_124M.bin -n $(n) 31373 612

benchmark: all
	./run_benchmark.sh

clean:
	rm -f gpt gpt_naive *.txt
