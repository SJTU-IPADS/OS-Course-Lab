CC = gcc
# -fno-tree-vectorize: GCC 12 and later vectorize some loops at -O2 on their
# own; this program is to run one weight at a time.
CFLAGS = -O2 -fno-tree-vectorize

# The file of the nano-quant lab (--recipe q4_0) and the question put to it.
MODEL = ../../../nano-quant/model/q4_0.gguf
PROMPT = 用一句话介绍你自己。

mini-ollama: mini_ollama.c model.c tokenizer.c mini_ollama.h
	$(CC) $(CFLAGS) mini_ollama.c model.c tokenizer.c -lm -o mini-ollama

run: mini-ollama
	./mini-ollama $(MODEL) "$(PROMPT)"

# The same file and the same question in ollama. The Modelfile keeps ollama
# on the CPU and on one thread. Needs a running server (ollama serve).
run-ollama:
	ollama create nq-q4-0 -f Modelfile
	ollama run nq-q4-0 --verbose "$(PROMPT)"

clean:
	rm -f mini-ollama

.PHONY: run run-ollama clean
