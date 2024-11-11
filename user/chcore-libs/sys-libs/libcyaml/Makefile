# SPDX-License-Identifier: ISC
#
# Copyright (C) 2017-2023 Michael Drake <tlsa@netsurf-browser.org>

# CYAML's versioning is <MAJOR>.<MINOR>.<PATCH>[-DEVEL]
# The main branch will usually be DEVEL.  The release process is:
# 0. Create branch for release.
# 1. Update the CHANGES.md file.
# 2. Set MAJOR, MINOR, and PATCH versions appropriately for changes.
# 3. Create PR, review, and merge to main.
# 4. Set VESION_DEVEL to 0, commit to main.
# 5. Tag the release: `git tag -a vN.N.N -m "libcyaml N.N.N"`
# 6. Set VESION_DEVEL to 1, commit to main.
VERSION_MAJOR = 2
VERSION_MINOR = 0
VERSION_PATCH = 0
VERSION_DEVEL = 1
VERSION_STR = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# Default variant depends on whether it's a development build.
ifeq ($(VERSION_DEVEL), 1)
	VARIANT = debug
else
	VARIANT = release
endif

# Unfortunately ASan is incompatible with valgrind, so we have a special
# variant for running with sanitisers.
VALID_VARIANTS := release debug san
ifneq ($(filter $(VARIANT),$(VALID_VARIANTS)),)
else
$(error VARIANT must be 'debug' (default), 'san', or 'release')
endif

UNAME_S := $(shell uname -s)

LIB_NAME = libcyaml
LIB_PKGCON = $(LIB_NAME).pc
LIB_STATIC = $(LIB_NAME).a
LIB_SHARED = $(LIB_NAME).so
LIB_SH_MAJ = $(LIB_SHARED).$(VERSION_MAJOR)
LIB_SH_VER = $(LIB_SHARED).$(VERSION_STR)

.IMPLICIT =

PREFIX ?= /usr/local
LIBDIR ?= lib
INCLUDEDIR ?= include

Q ?= @

CC ?= gcc
AR ?= ar
MKDIR =	mkdir -p
INSTALL ?= install -c
VALGRIND = valgrind --leak-check=full --track-origins=yes

VERSION_FLAGS = -DVERSION_MAJOR=$(VERSION_MAJOR) \
                -DVERSION_MINOR=$(VERSION_MINOR) \
                -DVERSION_PATCH=$(VERSION_PATCH) \
                -DVERSION_DEVEL=$(VERSION_DEVEL)

LIBYAML = yaml-0.1
LIBYAML_CFLAGS := $(if $(PKG_CONFIG),$(shell $(PKG_CONFIG) --cflags $(LIBYAML)),)
LIBYAML_LIBS := $(if $(PKG_CONFIG),$(shell $(PKG_CONFIG) --libs $(LIBYAML)),-lyaml)

INCLUDE = -I include
CPPFLAGS += $(VERSION_FLAGS) -MMD -MP
CFLAGS += $(INCLUDE) $(LIBYAML_CFLAGS)
CFLAGS += -std=c11 -Wall -Wextra -pedantic \
		-Wconversion -Wwrite-strings -Wcast-align -Wpointer-arith \
		-Winit-self -Wshadow -Wstrict-prototypes -Wmissing-prototypes \
		-Wredundant-decls -Wundef -Wvla -Wdeclaration-after-statement
LDFLAGS += $(LIBYAML_LIBS)
LDFLAGS_SHARED = -Wl,-soname=$(LIB_SH_MAJ) -shared

ifeq ($(VARIANT), debug)
	CFLAGS += -O0 -g
else ifeq ($(VARIANT), san)
	CFLAGS += -O0 -g -fsanitize=address -fsanitize=undefined -fno-sanitize-recover
	LDFLAGS += -fsanitize=address -fsanitize=undefined -fno-sanitize-recover
else
	CFLAGS += -O2 -DNDEBUG
endif

ifneq ($(filter coverage,$(MAKECMDGOALS)),)
	BUILDDIR = build/coverage/$(VARIANT)
	CFLAGS_COV = --coverage -DNDEBUG
	LDFLAGS_COV = --coverage
else
	BUILDDIR = build/$(VARIANT)
	CFLAGS_COV =
	LDFLAGS_COV =
endif

BUILDDIR_SHARED = $(BUILDDIR)/shared
BUILDDIR_STATIC = $(BUILDDIR)/static

LIB_SRC_FILES = mem.c free.c load.c save.c copy.c util.c utf8.c
LIB_SRC := $(addprefix src/,$(LIB_SRC_FILES))
LIB_OBJ = $(patsubst %.c,%.o, $(addprefix $(BUILDDIR)/,$(LIB_SRC)))
LIB_DEP = $(patsubst %.c,%.d, $(addprefix $(BUILDDIR)/,$(LIB_SRC)))
LIB_OBJ_SHARED = $(patsubst $(BUILDDIR)%,$(BUILDDIR_SHARED)%,$(LIB_OBJ))
LIB_OBJ_STATIC = $(patsubst $(BUILDDIR)%,$(BUILDDIR_STATIC)%,$(LIB_OBJ))
LIB_DEP_SHARED = $(patsubst $(BUILDDIR)%,$(BUILDDIR_SHARED)%,$(LIB_DEP))
LIB_DEP_STATIC = $(patsubst $(BUILDDIR)%,$(BUILDDIR_STATIC)%,$(LIB_DEP))

LIB_PATH = LD_LIBRARY_PATH=$(BUILDDIR)

ifeq ($(UNAME_S),Darwin)
	LIB_SHARED = $(LIB_NAME).dylib
	LIB_SH_MAJ = $(LIB_NAME).$(VERSION_MAJOR).dylib
	LIB_SH_VER = $(LIB_NAME).$(VERSION_STR).dylib
	LDFLAGS_SHARED = -dynamiclib -install_name "$(LIB_SH_MAJ)" -current_version $(VERSION_STR)
	LIB_PATH = DYLD_FALLBACK_LIBRARY_PATH=$(BUILDDIR)
endif

TEST_SRC_FILES = units/free.c units/load.c units/test.c units/util.c \
		units/errs.c units/file.c units/save.c units/copy.c \
		units/utf8.c
TEST_SRC := $(addprefix test/,$(TEST_SRC_FILES))
TEST_OBJ = $(patsubst %.c,%.o, $(addprefix $(BUILDDIR)/,$(TEST_SRC)))
TEST_DEP = $(patsubst %.c,%.d, $(addprefix $(BUILDDIR)/,$(TEST_SRC)))

TEST_BINS = \
		$(BUILDDIR)/test/units/cyaml-shared \
		$(BUILDDIR)/test/units/cyaml-static

all: $(BUILDDIR)/$(LIB_SH_MAJ) $(BUILDDIR)/$(LIB_STATIC) examples

coverage: test-verbose
	$(Q)$(MKDIR) $(BUILDDIR)
	$(Q)gcovr -e 'test/.*' -r .
	$(Q)gcovr -e 'test/.*' -x -o build/coverage.xml -r .
	$(Q)gcovr -e 'test/.*' --html --html-details -o build/coverage.html -r .

test test-quiet test-verbose test-debug: $(TEST_BINS)
	$(Q)for i in $(^); do $(LIB_PATH) $$i $(subst test,,$(subst test-,--,$@)) "$(TESTLIST)" || exit; done

valgrind valgrind-quiet valgrind-verbose valgrind-debug: $(TEST_BINS)
	$(Q)for i in $(^); do $(LIB_PATH) $(VALGRIND) $$i $(subst valgrind,,$(subst valgrind-,--,$@)) "$(TESTLIST)" || exit; done

check: test

$(BUILDDIR)/$(LIB_PKGCON): $(LIB_PKGCON).in
	sed \
		-e 's#PREFIX#$(PREFIX)#' \
		-e 's#LIBDIR#$(LIBDIR)#' \
		-e 's#INCLUDEDIR#$(INCLUDEDIR)#' \
		-e 's#VERSION#$(VERSION_STR)#' \
		$(LIB_PKGCON).in >$(BUILDDIR)/$(LIB_PKGCON)

$(BUILDDIR)/$(LIB_STATIC): $(LIB_OBJ_STATIC)
	$(AR) -rcs $@ $^

$(BUILDDIR)/$(LIB_SH_MAJ): $(LIB_OBJ_SHARED)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDFLAGS_COV) $(LDFLAGS_SHARED)

$(LIB_OBJ_STATIC): $(BUILDDIR_STATIC)/%.o : %.c
	$(Q)$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_COV) -c -o $@ $<

$(LIB_OBJ_SHARED): $(BUILDDIR_SHARED)/%.o : %.c
	$(Q)$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC $(CFLAGS_COV) -c -o $@ $<

docs:
	$(MKDIR) build/docs/api
	$(MKDIR) build/docs/devel
	doxygen docs/api.doxygen.conf
	doxygen docs/devel.doxygen.conf

clean:
	rm -rf build/

install: $(BUILDDIR)/$(LIB_SH_MAJ) $(BUILDDIR)/$(LIB_STATIC) $(BUILDDIR)/$(LIB_PKGCON)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(LIBDIR)
	$(INSTALL) $(BUILDDIR)/$(LIB_SH_MAJ) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(LIB_SH_VER)
	(cd $(DESTDIR)$(PREFIX)/$(LIBDIR) && { ln -s -f $(LIB_SH_VER) $(LIB_SH_MAJ) || { rm -f $(LIB_SH_MAJ) && ln -s $(LIB_SH_VER) $(LIB_SH_MAJ); }; })
	(cd $(DESTDIR)$(PREFIX)/$(LIBDIR) && { ln -s -f $(LIB_SH_VER) $(LIB_SHARED) || { rm -f $(LIB_SHARED) && ln -s $(LIB_SH_VER) $(LIB_SHARED); }; })
	$(INSTALL) $(BUILDDIR)/$(LIB_STATIC) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(LIB_STATIC)
	chmod 644 $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(LIB_STATIC)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)/cyaml
	$(INSTALL) -m 644 include/cyaml/* $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)/cyaml
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(LIBDIR)/pkgconfig
	$(INSTALL) -m 644 $(BUILDDIR)/$(LIB_PKGCON) $(DESTDIR)$(PREFIX)/$(LIBDIR)/pkgconfig/$(LIB_PKGCON)

examples: $(BUILDDIR)/planner $(BUILDDIR)/numerical

$(BUILDDIR)/planner: examples/planner/main.c $(BUILDDIR)/$(LIB_STATIC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/numerical: examples/numerical/main.c $(BUILDDIR)/$(LIB_STATIC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

-include $(LIB_DEP_SHARED) $(LIB_DEP_STATIC) $(TEST_DEP)

.PHONY: all test test-quiet test-verbose test-debug \
		valgrind valgrind-quiet valgrind-verbose valgrind-debug \
		clean coverage docs install examples check

$(BUILDDIR)/test/units/cyaml-static: $(TEST_OBJ) $(BUILDDIR)/$(LIB_STATIC)
	$(CC) $(LDFLAGS_COV) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/test/units/cyaml-shared: $(TEST_OBJ) $(BUILDDIR)/$(LIB_SH_MAJ)
	$(CC) $(LDFLAGS_COV) -o $@ $^ $(LDFLAGS)

$(TEST_OBJ): $(BUILDDIR)/%.o : %.c
	$(Q)$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_COV) -c -o $@ $<
