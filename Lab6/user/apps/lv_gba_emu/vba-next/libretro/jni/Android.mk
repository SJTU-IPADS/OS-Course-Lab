LOCAL_PATH := $(call my-dir)

CORE_DIR     := $(LOCAL_PATH)/../..
LIBRETRO_DIR := $(CORE_DIR)/libretro

SOURCES_ASM :=

FRONTEND_SUPPORTS_RGB565 := 1
TILED_RENDERING          := 1
LOAD_FROM_MEMORY         := 1

include $(CORE_DIR)/build/Makefile.common

COREFLAGS := -DHAVE_STDINT_H -DFRONTEND_SUPPORTS_RGB565 $(COREDEFINES) $(INCFLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_CXX) $(SOURCES_C) $(SOURCES_ASM)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(LIBRETRO_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
