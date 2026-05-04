ELF_TARGET_64 = echo

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
	-Wall -Wextra \
	-nostdlib \
	-no-pie \
	-m64 \
	-Iinclude \
	-I$(SYSROOT)/include \
	-I$(SYSROOT)/libc \
	-nostdinc

LDFLAGS = \
	-static \
	-nostdlib \
	-no-pie \
	-m64 \
	-L$(LIB_DIR)

SRCS = $(shell find src -name '*.c')
ELF_OBJS_64 = $(patsubst src/%.c,$(ELF_OBJ_DIR_64)/%.o,$(SRCS))

all: $(ELF_TARGET_64)

$(ELF_TARGET_64): $(ELF_OBJS_64)
	@mkdir -p $(ELF_BIN_DIR_64)
	$(CC) $(LDFLAGS) \
		-o $(ELF_BIN_DIR_64)/$(ELF_TARGET_64) \
		$(CRT0) \
		$(ELF_OBJS_64) \
		-l:blibc.a

$(ELF_OBJ_DIR_64)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(COMMON_CFLAGS) -c $< -o $@

clean:
	rm -rf bin

.PHONY: all clean
