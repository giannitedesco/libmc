.SUFFIXES:

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar
DEL := rm -f

EXTRA_DEFS := -D_FILE_OFFSET_BITS=64 -Iinclude
CFLAGS := -g -pipe -O2 -Wall \
	-Wsign-compare -Wcast-align \
	-Waggregate-return \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wmissing-noreturn \
	-finline-functions \
	-Wmissing-format-attribute \
	-fwrapv \
	-Iinclude \
	$(EXTRA_DEFS) 

LIBMC_SLIB := libmc.a
LIBMC_OBJ := dim.o \
		region.o \
		chunk.o \
		nbt.o \
		hgang.o

MCDUMP_BIN := mcdump
MCDUMP_LIBS := -lz
MCDUMP_SLIBS := $(LIBMC_SLIB)
MCDUMP_OBJ := mcdump.o

ALL_BIN := $(MCDUMP_BIN) $(LIBMC_SLIB)
ALL_OBJ := $(MCDUMP_OBJ) $(LIBMC_OBJ)
ALL_DEP := $(patsubst %.o, .%.d, $(ALL_OBJ))
ALL_TARGETS := $(ALL_BIN)

TARGET: all

.PHONY: all clean

all: $(ALL_BIN)

ifeq ($(filter clean, $(MAKECMDGOALS)),clean)
CLEAN_DEP := clean
else
CLEAN_DEP :=
endif

%.o %.d: %.c $(CLEAN_DEP) $(ROOT_DEP) Makefile
	@echo " [C] $<"
	@$(CC) $(CFLAGS) -MMD -MF $(patsubst %.o, .%.d, $@) \
		-MT $(patsubst .%.d, %.o, $@) \
		-c -o $(patsubst .%.d, %.o, $@) $<

$(LIBMC_SLIB): $(LIBMC_OBJ)
	@echo " [SLIB] $@"
	@$(AR) cr $@ $^

$(MCDUMP_BIN): $(MCDUMP_OBJ) $(MCDUMP_SLIBS)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(MCDUMP_LIBS)
clean:
	$(DEL) $(ALL_TARGETS) $(ALL_OBJ) $(ALL_DEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(ALL_DEP)
endif
