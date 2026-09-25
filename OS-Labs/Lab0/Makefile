LAB := 0
TIMEOUT := 10

include $(CURDIR)/../Scripts/lab.mk

export LABROOT LABDIR LAB TIMEOUT

bomb: student-number.txt
	@echo "  GEN bomb"
	@$(CURDIR)/scripts/generate_bomb.sh

clean:
	@echo "  CLEAN bomb"
	@rm -f bomb

## execute bomb
qemu:
	@$(QEMU) bomb

qemu-gdb:
	@$(QEMU) -g 1234 bomb

qemu-grade:
	@touch ans.txt
	@sed -E -i 's|\r$$||g' ans.txt
	@$(QEMU) bomb < ans.txt

gdb:
	$(GDB) -ex "set architecture aarch64" -ex "target remote localhost:1234" -ex "add-symbol-file bomb"

grade:
	@$(DOCKER_RUN) $(GRADER) -t $(TIMEOUT) -f $(CURDIR)/scores.json make qemu-grade

submit:
	@echo "  SUBMIT lab0"
	@grep -q '[^[:space:]]' < student-number.txt && echo "  Student number: $(shell cat student-number.txt)" || (echo "  Student number not found" && exit 1)
	@tar -caf lab$(LAB).tar.gz ans.txt student-number.txt

.PHONY: qemu qemu-grade qemu-gdb gdb grade clean bomb
