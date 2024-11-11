cc = gcc
prom = gdbserver
obj = gdbserver.o utils.o packets.o signals.o
cflags = -std=gnu99

$(prom): $(obj)
	$(cc) $(cflags) -o $(prom) $(obj)

gdbserver.o : gdbserver.c arch.h utils.h packets.h gdb_signals.h
	$(cc) $(cflags) -c $< -o $@

signals.o : signals.c gdb_signals.h gdb/signals.h gdb/signals.def
	$(cc) $(cflags) -c $< -o $@

%.o: %.c
	$(cc) $(cflags) -c $< -o $@
