BIN_DIR    := bin
BLIBC_REPO  = https://github.com/Bleed-Kernel/blibc.git
BLIBC_DIR   = external/blibc
SYSROOT     = sysroot

PROGRAM_MAKEFILES := $(wildcard */Makefile)
PROGRAMS := $(patsubst %/Makefile,%,$(PROGRAM_MAKEFILES))

.PHONY: all clean distclean blibc $(PROGRAMS)

all: blibc $(PROGRAMS)

blibc:
	@echo "[BLIBC] Preparing blibc"
	@if [ ! -d "$(BLIBC_DIR)" ]; then \
		git clone $(BLIBC_REPO) $(BLIBC_DIR); \
	fi
	@$(MAKE) -C $(BLIBC_DIR)
	@echo "[BLIBC] Syncing sysroot"
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
	@rm -rf $(BIN_DIR) $(SYSROOT) external