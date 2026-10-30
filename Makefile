# Run from the webserver/ directory.
SHELL := /bin/bash
CC := gcc

BIN_DIR := bin
VAR_DIR := var
SSL_DIR ?= cert
WEBUI_DIR := lib/src/libcwebui
WEBUI_SO := $(WEBUI_DIR)/lib/lib/libWebserver.so
WEBUI_LIB := lib/libWebserver.so

SRC := $(wildcard src/*.c src/third_party/*.c src/doms/*.c)
HEADERS := $(wildcard include/*.h lib/include/*.h $(WEBUI_DIR)/lib/include/*.h)

CPPFLAGS += -Iinclude -Ilib/include -I$(WEBUI_DIR)/lib/include
CFLAGS += -Wall -pthread
LDFLAGS += -Llib -Wl,-rpath,'$$ORIGIN/../lib'
LDLIBS += -lWebserver -lssl -lcrypto -pthread

BUILDS := release_http release_https debug_http mem_debug_http debug_https mem_debug_https
BUILD_FILES := $(addprefix $(BIN_DIR)/,$(BUILDS))

.DEFAULT_GOAL := all
.PHONY: all release pyromelonz.net release_http release_https debug_http \
        mem_debug_http debug_https mem_debug_https ws_deps ssl launch clean clean-webui

all: release

$(BIN_DIR) $(VAR_DIR):
	mkdir -p "$@"

$(VAR_DIR)/uploads: | $(VAR_DIR)
	mkdir -p "$@"

$(VAR_DIR)/version.txt: | $(VAR_DIR)
	printf '%s\n' '1.0.0' > "$@"

# The nested Makefile generates this .so, not the top-level Makefile.
$(WEBUI_SO):
	$(MAKE) -C "$(WEBUI_DIR)/lib"
	@test -f "$@" || { echo "Missing $(WEBUI_SO) after build" >&2; exit 1; }

# Link from lib/ (which is a symlink to ../lib in this tree).
$(WEBUI_LIB): $(WEBUI_SO)
	ln -sfn src/libcwebui/lib/lib/libWebserver.so "$@"

ws_deps: $(WEBUI_LIB) | $(BIN_DIR) $(VAR_DIR)/uploads $(VAR_DIR)/version.txt

ssl:
	@if [ ! -d "$(SSL_DIR)" ]; then \
		echo "ERROR: SSL certificate directory missing: $(SSL_DIR)" >&2; \
		echo "Pass SSL_DIR=/path/to/certs or create the cert symlink." >&2; \
		exit 1; \
	fi

# Target-specific flags. Only HTTPS variants need the certificate directory.
$(BIN_DIR)/release_https: EXTRA_CFLAGS := -DUSE_SSL
$(BIN_DIR)/debug_http: EXTRA_CFLAGS := -ggdb -DLOG_VERBOSITY=1 -DDEBUG_MEM
$(BIN_DIR)/mem_debug_http: EXTRA_CFLAGS := -ggdb -DLOG_VERBOSITY=0 -DDEBUG_MEM_V
$(BIN_DIR)/debug_https: EXTRA_CFLAGS := -ggdb -DUSE_SSL -DLOG_VERBOSITY=1 -DDEBUG_MEM
$(BIN_DIR)/mem_debug_https: EXTRA_CFLAGS := -ggdb -DUSE_SSL -DLOG_VERBOSITY=0 -DDEBUG_MEM_V

$(BIN_DIR)/release_https $(BIN_DIR)/debug_https $(BIN_DIR)/mem_debug_https: | ssl

# Link libraries after the source files, so symbols resolve in the right order.
$(BUILD_FILES): $(SRC) $(HEADERS) $(WEBUI_LIB) | $(BIN_DIR) $(VAR_DIR)/uploads $(VAR_DIR)/version.txt
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(SRC) -o "$@" $(LDFLAGS) $(LDLIBS)

release_http: $(BIN_DIR)/release_http
release_https: $(BIN_DIR)/release_https
debug_http: $(BIN_DIR)/debug_http
mem_debug_http: $(BIN_DIR)/mem_debug_http
debug_https: $(BIN_DIR)/debug_https
mem_debug_https: $(BIN_DIR)/mem_debug_https

release: $(BIN_DIR)/pyromelonz.net
pyromelonz.net: $(BIN_DIR)/pyromelonz.net

$(BIN_DIR)/pyromelonz.net: $(BIN_DIR)/release_https | $(BIN_DIR)
	cp -f "$<" "$@"

launch: $(BIN_DIR)/pyromelonz.net
	scp "$<" dev@pyromelonz.net:webserver/bin/pyromelonz.net
	curl http://pyromelonz.net/shutdown_dev

# Building and cleaning must not remove user uploads or kill a running server.
clean:
	rm -rf "$(BIN_DIR)"

clean_user: $(VAR_DIR)/*.dat
	rm -f var/user_data.dat
	rm -f var/user_reg.dat

clean-webui:
	$(MAKE) -C "$(WEBUI_DIR)/lib" clean
	rm -f "$(WEBUI_LIB)"
