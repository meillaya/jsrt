# jsrt build.
#
# The plain C targets build the CLI, the unit tests, and the static library
# with the host toolchain. Zig is only needed by the optional verification
# lanes near the end of this file.

.DELETE_ON_ERROR:
.DEFAULT_GOAL := all

# Toolchain. Override CC, AR, CFLAGS, LDFLAGS, and LDLIBS from the command
# line or the environment.
CC ?= cc
AR ?= ar
ARFLAGS = rcs
CFLAGS ?= -O2
LDFLAGS ?=
LDLIBS ?= -lm

# Required language and warning flags. These run on every compile.
REQUIRED_CFLAGS = -std=c17 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?=
INCLUDES = -Iinclude

# Zig is required only by the debug and asan lanes.
ZIG ?= zig
ZIG_LOCAL_CACHE_DIR ?= $(CURDIR)/.zig-cache
ZIG_GLOBAL_CACHE_DIR ?= $(CURDIR)/.zig-cache/global
ZIG_BUILD = ZIG_GLOBAL_CACHE_DIR="$(ZIG_GLOBAL_CACHE_DIR)" $(ZIG) build --cache-dir "$(ZIG_LOCAL_CACHE_DIR)"

# Install layout. DESTDIR stages the install tree and does not change the
# paths written into jsrt.pc.
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig
DESTDIR ?=

# Version written into jsrt.pc. It comes from the public header.
JSRT_VERSION ?= $(shell sed -n 's/^#define JSRT_VERSION_\(MAJOR\|MINOR\|PATCH\) //p' include/jsrt/runtime.h | paste -s -d.)

# Benchmark knobs. They match the script options.
ITERATIONS ?= 5
WARMUPS ?= 1
FIXTURE ?=
FILTER ?=

# Sources. The runtime list is every file under src except the CLI entry point.
SRC_FILES := $(wildcard src/*.c)
RUNTIME_SRCS := $(filter-out src/main.c,$(SRC_FILES))
TEST_SRCS := $(wildcard tests/*.c)

RUNTIME_OBJS := $(patsubst src/%.c,build/obj/%.o,$(RUNTIME_SRCS))
MAIN_OBJ := build/obj/main.o
TEST_OBJS := $(patsubst tests/%.c,build/obj/tests/%.o,$(TEST_SRCS))
DEPS := $(RUNTIME_OBJS:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJS:.o=.d)

CLI_BINARY := build/jsrt
TEST_BINARY := build/test_jsrt
STATIC_LIB := build/libjsrt.a
PC_FILE := build/jsrt.pc

.PHONY: all lib test install uninstall clean help debug asan conformance test262-adapter vm-parity prose cli-test verify-production bench bench-vm bench-compare bench-smoke FORCE

all: $(CLI_BINARY) $(TEST_BINARY) $(STATIC_LIB)

lib: $(STATIC_LIB)

test: $(TEST_BINARY)
	./$(TEST_BINARY)

$(STATIC_LIB): $(RUNTIME_OBJS) | build
	$(AR) $(ARFLAGS) $@ $^

$(CLI_BINARY): $(MAIN_OBJ) $(STATIC_LIB) | build
	$(CC) $(LDFLAGS) -o $@ $(MAIN_OBJ) $(STATIC_LIB) $(LDLIBS)

$(TEST_BINARY): $(TEST_OBJS) $(STATIC_LIB) | build
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJS) $(STATIC_LIB) $(LDLIBS)

build:
	mkdir -p $@

build/obj: | build
	mkdir -p $@

build/obj/tests: | build/obj
	mkdir -p $@

# Dependency files come from -MMD -MP next to each object.
build/obj/%.o: src/%.c | build/obj
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(REQUIRED_CFLAGS) -MMD -MP -c $< -o $@

build/obj/tests/%.o: tests/%.c | build/obj/tests
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(REQUIRED_CFLAGS) -MMD -MP -c $< -o $@

# The pkg-config file always uses the values from the current make invocation.
$(PC_FILE): jsrt.pc.in FORCE | build
	sed -e 's|@PREFIX@|$(PREFIX)|g' \
	    -e 's|@LIBDIR@|$(LIBDIR)|g' \
	    -e 's|@INCLUDEDIR@|$(INCLUDEDIR)|g' \
	    -e 's|@VERSION@|$(JSRT_VERSION)|g' \
	    jsrt.pc.in > $@

install: $(CLI_BINARY) $(STATIC_LIB) $(PC_FILE)
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(LIBDIR)"
	install -d "$(DESTDIR)$(INCLUDEDIR)/jsrt" "$(DESTDIR)$(PKGCONFIGDIR)"
	install -m 755 $(CLI_BINARY) "$(DESTDIR)$(BINDIR)/jsrt"
	install -m 644 $(STATIC_LIB) "$(DESTDIR)$(LIBDIR)/libjsrt.a"
	install -m 644 include/jsrt/*.h "$(DESTDIR)$(INCLUDEDIR)/jsrt/"
	install -m 644 $(PC_FILE) "$(DESTDIR)$(PKGCONFIGDIR)/jsrt.pc"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/jsrt" "$(DESTDIR)$(LIBDIR)/libjsrt.a"
	rm -f "$(DESTDIR)$(PKGCONFIGDIR)/jsrt.pc"
	rm -f "$(DESTDIR)$(INCLUDEDIR)"/jsrt/*.h
	-rmdir "$(DESTDIR)$(INCLUDEDIR)/jsrt"

# Verification lanes. These use the local plain C build and the scripts.
conformance: $(CLI_BINARY)
	BIN=./$(CLI_BINARY) bash scripts/conformance.sh

test262-adapter:
	bash scripts/test262-adapter.sh

vm-parity: $(CLI_BINARY)
	BIN=./$(CLI_BINARY) bash scripts/vm-parity.sh

prose:
	bash scripts/prose.sh

# The CLI test harness lands in scripts/cli.sh. Include it in the production
# gate automatically once the file exists.
cli-test: $(CLI_BINARY)
	BIN=./$(CLI_BINARY) bash scripts/cli.sh

ifneq ($(wildcard scripts/cli.sh),)
VERIFY_DEPS = cli-test
endif

verify-production: test conformance test262-adapter vm-parity prose $(VERIFY_DEPS)
	@echo "Production verification passed"

BENCH_ENV = BINARY=./$(CLI_BINARY) ITERATIONS=$(ITERATIONS) WARMUPS=$(WARMUPS) FIXTURE='$(FIXTURE)' FILTER='$(FILTER)'

bench: $(CLI_BINARY)
	$(BENCH_ENV) bash scripts/benchmark.sh bench

bench-vm: $(CLI_BINARY)
	$(BENCH_ENV) bash scripts/benchmark.sh bench-vm

bench-compare: $(CLI_BINARY)
	$(BENCH_ENV) bash scripts/benchmark.sh bench-compare

bench-smoke: $(CLI_BINARY)
	BINARY=./$(CLI_BINARY) ITERATIONS=1 WARMUPS=0 FIXTURE='$(FIXTURE)' FILTER='$(FILTER)' bash scripts/benchmark.sh bench-smoke

# Debug and sanitizer builds still come from build.zig.
debug:
	$(ZIG_BUILD) debug

asan:
	$(ZIG_BUILD) asan

clean:
	rm -rf build/obj
	rm -f $(CLI_BINARY) $(TEST_BINARY) $(STATIC_LIB) $(PC_FILE)
	rm -rf zig-out

help:
	@echo "Usage: make [target] [VAR=value]"
	@echo ""
	@echo "Plain C targets:"
	@echo "  all        Build the CLI, the unit tests, and the static library"
	@echo "  lib        Build build/libjsrt.a"
	@echo "  test       Build and run the unit tests"
	@echo "  install    Install the CLI, libjsrt.a, headers, and jsrt.pc"
	@echo "  uninstall  Remove installed files"
	@echo "  clean      Remove local build outputs"
	@echo ""
	@echo "Verification targets:"
	@echo "  conformance       Run the local conformance manifest"
	@echo "  test262-adapter   Validate the Test262 subset manifest"
	@echo "  vm-parity         Run the tree walker and VM parity fixtures"
	@echo "  prose             Run the house prose gate"
	@echo "  cli-test          Run the CLI behavior harness"
	@echo "  verify-production Run test, conformance, test262-adapter, vm-parity, prose, and cli-test"
	@echo "  bench             Run the tree walker benchmark suite"
	@echo "  bench-vm          Run the VM benchmark suite"
	@echo "  bench-compare     Compare tree walker and VM timings"
	@echo "  bench-smoke       Run a quick benchmark smoke check"
	@echo "  debug             Build debug binaries through zig build"
	@echo "  asan              Build and test with AddressSanitizer through zig build"
	@echo ""
	@echo "Install variables:"
	@echo "  PREFIX=$(PREFIX)  BINDIR=$(BINDIR)"
	@echo "  LIBDIR=$(LIBDIR)  INCLUDEDIR=$(INCLUDEDIR)"
	@echo "  PKGCONFIGDIR=$(PKGCONFIGDIR)  DESTDIR=$(DESTDIR)"

FORCE:

-include $(DEPS)
