# Note that this file should be included directly in every Makefile inside each lab's folder.
# This sets up the environment variable for lab's Makefile.

ifndef PROJECT
PROJECT:=(shell git rev-parse --show-toplevel)
endif

# Toolchain configuration
GDB?=gdb

ifeq (,$(wildcard $(PROJECT)/Scripts/env_generated.mk))
$(error Please run fixdeps to create the environment first!)
endif
