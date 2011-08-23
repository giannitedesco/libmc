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

LIBMC_LIB := libmc.a
LIBMC_OBJ := world.o \
		dim.o \
		region.o \
		chunk.o \
		level.o \
		schematic.o \
		nbt.o \
		hgang.o

MCDUMP_BIN := mcdump
MCDUMP_LIBS := -lz
MCDUMP_SLIBS := $(LIBMC_LIB)
MCDUMP_OBJ := mcdump.o

NBTDUMP_BIN := nbtdump
NBTDUMP_LIBS := -lz
NBTDUMP_SLIBS := $(LIBMC_LIB)
NBTDUMP_OBJ := nbtdump.o

MKREGION_BIN := mkregion
MKREGION_LIBS := -lz
MKREGION_SLIBS := $(LIBMC_LIB)
MKREGION_OBJ := mkregion.o

MKWORLD_BIN := mkworld
MKWORLD_LIBS := -lz
MKWORLD_SLIBS := $(LIBMC_LIB)
MKWORLD_OBJ := mkworld.o

ALL_BIN := $(MCDUMP_BIN) $(NBTDUMP_BIN) $(LIBMC_LIB) \
		$(MKREGION_BIN) $(MKWORLD_BIN)
ALL_OBJ := $(MCDUMP_OBJ) $(NBTDUMP_OBJ) $(LIBMC_OBJ) \
		$(MKREGION_OBJ) $(MKWORLD_OBJ)
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

$(LIBMC_LIB): $(LIBMC_OBJ)
	@echo " [SLIB] $@"
	@$(AR) cr $@ $^

$(MCDUMP_BIN): $(MCDUMP_OBJ) $(MCDUMP_SLIBS)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(MCDUMP_LIBS)

$(NBTDUMP_BIN): $(NBTDUMP_OBJ) $(NBTDUMP_SLIBS)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(NBTDUMP_LIBS)

$(MKREGION_BIN): $(MKREGION_OBJ) $(MKREGION_SLIBS)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(MKREGION_LIBS)

$(MKWORLD_BIN): $(MKWORLD_OBJ) $(MKWORLD_SLIBS)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(MKWORLD_LIBS)

clean:
	$(DEL) $(ALL_TARGETS) $(ALL_OBJ) $(ALL_DEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(ALL_DEP)
endif
