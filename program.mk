# shared by every tool, a tool's Makefile is just ELF_TARGET_64 = name then include ../program.mk

CC := x86_64-elf-gcc

SYSROOT := ../sysroot

ELF_BIN_DIR_64 = bin
ELF_OBJ_DIR_64 = $(ELF_BIN_DIR_64)/obj

LIB_DIR = $(SYSROOT)/lib
CRT0    = $(LIB_DIR)/start.o

COMMON_CFLAGS = \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-ffunction-sections \
	-fdata-sections \
	-O2 \
	-Wall -Wextra \
	-nostdlib \
	-no-pie \
	-m64 \
	-MMD -MP \
	-I../common \
	-I$(SYSROOT)/include \
	-I$(SYSROOT)/include/bleed \
	-nostdinc

LDFLAGS = \
	-static \
	-nostdlib \
	-no-pie \
	-m64 \
	-Wl,--gc-sections \
	-L$(LIB_DIR)

SRCS        = $(shell find src -name '*.c')
COMMON_SRCS = $(wildcard ../common/*.c)
ELF_OBJS_64 = $(patsubst src/%.c,$(ELF_OBJ_DIR_64)/%.o,$(SRCS)) \
              $(patsubst ../common/%.c,$(ELF_OBJ_DIR_64)/common/%.o,$(COMMON_SRCS))

all: $(ELF_BIN_DIR_64)/$(ELF_TARGET_64)

$(ELF_BIN_DIR_64)/$(ELF_TARGET_64): $(ELF_OBJS_64) $(LIB_DIR)/blibc.a
	@mkdir -p $(ELF_BIN_DIR_64)
	$(CC) $(LDFLAGS) -o $@ $(CRT0) $(ELF_OBJS_64) -l:blibc.a -lgcc

$(ELF_OBJ_DIR_64)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(COMMON_CFLAGS) -c $< -o $@

$(ELF_OBJ_DIR_64)/common/%.o: ../common/%.c
	@mkdir -p $(dir $@)
	$(CC) $(COMMON_CFLAGS) -c $< -o $@

-include $(ELF_OBJS_64:.o=.d)

clean:
	rm -rf bin

.PHONY: all clean
