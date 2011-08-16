.SUFFIXES:

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

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

MCDUMP_BIN := mcdump
MCDUMP_LIBS := 
MCDUMP_OBJ = mcdump.o \
		region.o \
		chunk.o

ALL_BIN := $(MCDUMP_BIN)
ALL_OBJ := $(MCDUMP_OBJ)
ALL_DEP := $(patsubst %.o, .%.d, $(ALL_OBJ))
ALL_TARGETS := $(ALL_BIN)

TARGET: all

.PHONY: all clean walk

all: $(ALL_BIN)

ifeq ($(filter clean, $(MAKECMDGOALS)),clean)
CLEAN_DEP := clean
else
CLEAN_DEP :=
endif

ifeq ($(filter root, $(MAKECMDGOALS)),root)
ROOT_DEP := root
else
ROOT_DEP :=
endif

%.o %.d: %.c $(CLEAN_DEP) $(ROOT_DEP) Makefile
	@echo " [C] $<"
	@$(CC) $(CFLAGS) -MMD -MF $(patsubst %.o, .%.d, $@) \
		-MT $(patsubst .%.d, %.o, $@) \
		-c -o $(patsubst .%.d, %.o, $@) $<

$(MCDUMP_BIN): $(MCDUMP_OBJ)
	@echo " [LINK] $@"
	@$(CC) $(CFLAGS) -o $@ $(MCDUMP_OBJ) $(MCDUMP_LIBS)

clean:
	rm -f $(ALL_TARGETS) $(ALL_OBJ) $(ALL_DEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(ALL_DEP)
endif
