LAB := 0

PROJECT := $(CURDIR)/..
GRADER := $(PROJECT)/Scripts/grader.sh

export PROJECT CURDIR LAB

include $(PROJECT)/Scripts/lab.mk

bomb: student-number.txt
	@echo "  GEN bomb"
	@$(CURDIR)/Scripts/generate_bomb.sh

clean:
	@echo "  CLEAN bomb"
	@rm -f bomb

## execute bomb
qemu:
	@qemu-aarch64 bomb

qemu-gdb:
	@qemu-aarch64 -g 1234 bomb

qemu-grade:
	@qemu-aarch64 bomb < ans.txt

gdb:
	$(GDB) -ex "set architecture aarch64" -ex "target remote localhost:1234" -ex "add-symbol-file bomb"

grade:
	@$(GRADER)

submit:
	@echo "  SUBMIT lab0"
	@grep -q '[^[:space:]]' < student-number.txt && echo "  Student number: $(shell cat student-number.txt)" || (echo "  Student number not found" && exit 1)
	@tar -caf lab$(LAB).tar.gz ans.txt student-number.txt

.PHONY: qemu qemu-grade qemu-gdb gdb grade clean bomb
