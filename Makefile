BIN_DIR := bin

BLIBC_DIR ?= ../blibc
SYSROOT    = sysroot

PROGRAM_MAKEFILES := $(wildcard */Makefile)
PROGRAMS := $(patsubst %/Makefile,%,$(PROGRAM_MAKEFILES))

.PHONY: all clean distclean blibc $(PROGRAMS)

all: blibc $(PROGRAMS)

blibc:
	@echo "[BLIBC] Preparing blibc"
	@$(MAKE) -C $(BLIBC_DIR)
	@mkdir -p $(SYSROOT)
	@cp -r $(BLIBC_DIR)/sysroot/* $(SYSROOT)/

$(PROGRAMS):
	@echo "[COREUTILS] Building $@"
	@$(MAKE) -C $@ SYSROOT=../$(SYSROOT)
	@if [ -f "$@/bin/$@" ]; then \
		mkdir -p $(BIN_DIR); \
		cp "$@/bin/$@" "$(BIN_DIR)/$@"; \
	else \
		echo "ERROR: $@/bin/$@ not found"; \
		exit 1; \
	fi

clean:
	@for dir in $(PROGRAMS); do \
		$(MAKE) -C $$dir clean; \
	done
	@rm -rf $(BIN_DIR)

distclean:
	@rm -rf $(BIN_DIR) $(SYSROOT)
