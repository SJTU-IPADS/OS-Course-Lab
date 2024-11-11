PROJECT=libboundscheck.so

CC?=gcc

OPTION  = -fPIC
OPTION += -fstack-protector-all
OPTION += -D_FORTIFY_SOURCE=2 -O2
OPTION += -Wformat=2 -Wfloat-equal -Wshadow
OPTION += -Wconversion
OPTION += -Wformat-security
OPTION += -Wextra
OPTION += --param ssp-buffer-size=4
OPTION += -Warray-bounds
OPTION += -Wpointer-arith
OPTION += -Wcast-qual
OPTION += -Wstrict-prototypes
OPTION += -Wmissing-prototypes
OPTION += -Wstrict-overflow=1
OPTION += -Wstrict-aliasing=2
OPTION += -Wswitch -Wswitch-default

CFLAG   =  -Wall -DNDEBUG -O2 $(OPTION)

SOURCES=$(wildcard src/*.c)

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

.PHONY:clean

CFLAG += -Iinclude
LD_FLAG = -fPIC -s -Wl,-z,relro,-z,now,-z,noexecstack -fstack-protector-all

$(PROJECT): $(OBJECTS)
	mkdir -p lib
	$(CC)  -shared -o lib/$@ $(patsubst %.o,obj/%.o,$(notdir $(OBJECTS))) $(LD_FLAG)
	@echo "finish $(PROJECT)"

.c.o:
	@mkdir -p obj
	$(CC) -c $< $(CFLAG) -o obj/$(patsubst %.c,%.o,$(notdir $<))

clean:
	-rm -rf obj lib
	@echo "clean up"
