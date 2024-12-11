include $(CURDIR)/filelist.mk

FILES := $(FILES) $(shell find . -name "*.pdf" \
		 -o -name "*.doc" \
		 -o -name "*.docx")
submit:
	$(Q)tar -czf lab$(LAB).tar.gz $(FILES) | :
	$(Q)echo "  Submit Lab$(LAB)"

.PHONY: submit
