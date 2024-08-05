# .PHONY: gdb qemu qemu-gdb clean
## compile

bomb: student-number.txt
	./generate_bomb.sh

clean:
	rm -f bomb

## execute bomb
qemu: bomb
	qemu-aarch64 bomb

qemu-gdb: bomb
	qemu-aarch64 -g 1234 bomb

gdb:
	gdb-multiarch -ex "set architecture aarch64" -ex "target remote localhost:1234" -ex "add-symbol-file bomb"
