include $(CURDIR)/filelist.mk

submit:
	$(Q)tar -czf lab$(LAB).tar.gz $(FILES)
	$(Q)echo "  Submit Lab$(LAB)"

.PHONY: submit
