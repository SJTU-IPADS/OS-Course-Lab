# Note that this file should be included directly in every Makefile inside each lab's folder.
# This sets up the environment variable for lab's Makefile.

ifndef PROJECT
PROJECT := $(shell git rev-parse --show-toplevel)
endif

# Toolchain configuration
GDB ?= gdb

ifeq (,$(wildcard $(PROJECT)/Scripts/env_generated.mk))
	$(error Please run fixdeps to create the environment first!)
endif

ifeq (,$(LAB))
$(error LAB is not set!)
endif

ifeq ($(shell test $(LAB) -eq 0; echo $$?),1)
	ifeq ($(shell test $(LAB) -gt 5; echo $$?),0)
		include $(PROJECT)/Scripts/extras.mk
	else
		include $(PROJECT)/Scripts/kernel.mk
	endif
	include $(PROJECT)/Scripts/submit.mk
endif
