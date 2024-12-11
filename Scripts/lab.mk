# Note that this file should be included directly in every Makefile inside each lab's folder.
# This sets up the environment variable for lab's Makefile.

ifndef LABROOT
LABROOT := $(CURDIR)/..
endif

SCRIPTS := $(LABROOT)/Scripts
ifeq (,$(wildcard $(SCRIPTS)/env_generated.mk))
$(error Please run $(SCRIPTS)/gendeps.sh to create the environment first!)
endif

ifeq (,$(LAB))
$(error LAB is not set!)
endif


LABDIR  := $(LABROOT)/Lab$(LAB)
SCRIPTS := $(LABROOT)/Scripts
GRADER  ?= $(SCRIPTS)/grader.sh

include $(SCRIPTS)/env_generated.mk


# Toolchain configuration
GDB ?= gdb
DOCKER ?= docker
DOCKER_IMAGE ?= ipads/oslab:24.09
ifeq (,$(wildcard /docker.env))
DOCKER_RUN ?= 
else
DOCKER_RUN ?= $(DOCKER) run -it --rm \
		-e SCRIPTS=$(SCRIPTS) \
		-e LABROOT=$(LABROOT) \
		-e LABDIR=$(LABDIR) \
		-e TIMEOUT=$(TIMEOUT) \
		-e LAB=$(LAB) \
		-u $(shell id -u $(USER)):$(shell id -g $(USER)) \
		-v $(LABROOT):$(LABROOT) -w $(CURDIR) \
		--security-opt=seccomp:unconfined \
    --platform=linux/amd64 \
		ipads/oslab:24.09
endif
QEMU-SYS ?= qemu-system-aarch64
QEMU-USER ?= qemu-aarch64

# Timeout for grading
TIMEOUT ?= 10

ifeq ($(shell test $(LAB) -eq 0; echo $$?),1)
	QEMU := $(QEMU-SYS)
	ifeq ($(shell test $(LAB) -gt 4; echo $$?),0)
		include $(LABROOT)/Scripts/extras/lab$(LAB).mk
	else
		include $(LABROOT)/Scripts/kernel.mk
	endif
	include $(LABROOT)/Scripts/submit.mk
else
	QEMU := $(QEMU-USER)
endif
