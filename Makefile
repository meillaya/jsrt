CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN := $(BUILD_DIR)/jsrt
TEST_BIN := $(BUILD_DIR)/test_jsrt
BENCH_SCRIPT := scripts/benchmark.sh
ITERATIONS ?= 5
WARMUPS ?= 1
FIXTURE ?=
FILTER ?=

SRC := src/main.c src/memory.c src/diagnostics.c src/lexer.c src/ast.c src/parser.c src/value.c src/env.c src/gc.c src/eval.c src/runtime.c src/bytecode.c src/compiler.c src/vm.c
LIB_SRC := src/memory.c src/diagnostics.c src/lexer.c src/ast.c src/parser.c src/value.c src/env.c src/gc.c src/eval.c src/runtime.c src/bytecode.c src/compiler.c src/vm.c
TEST_SRC := tests/test_runner.c tests/test_lexer.c tests/test_parser.c tests/test_eval.c tests/test_runtime.c tests/test_diagnostics.c tests/test_bytecode.c tests/test_vm.c

OBJ := $(SRC:src/%.c=$(OBJ_DIR)/src/%.o)
TEST_OBJ := $(LIB_SRC:src/%.c=$(OBJ_DIR)/src/%.o) $(TEST_SRC:tests/%.c=$(OBJ_DIR)/tests/%.o)

.PHONY: all clean test debug conformance test262-adapter vm-parity verify-production bench bench-vm bench-compare bench-smoke

all: $(BIN)

debug: CFLAGS += -O0 -DJSRT_DEBUG
debug: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_BIN): $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(BIN) $(TEST_BIN)
	./$(TEST_BIN)

conformance: $(BIN) scripts/conformance.sh conformance/manifest.csv
	@scripts/conformance.sh

clean:
	rm -rf $(BUILD_DIR)


test262-adapter: scripts/test262-adapter.sh conformance/test262-subset-manifest.csv
	@scripts/test262-adapter.sh

vm-parity: $(BIN) scripts/vm-parity.sh conformance/vm-parity-manifest.csv
	@scripts/vm-parity.sh

verify-production: test conformance test262-adapter vm-parity

bench: $(BIN) $(BENCH_SCRIPT) benchmarks/manifest.csv
	@ITERATIONS=$(ITERATIONS) WARMUPS=$(WARMUPS) FIXTURE="$(FIXTURE)" FILTER="$(FILTER)" ./$(BENCH_SCRIPT) bench

bench-vm: $(BIN) $(BENCH_SCRIPT) benchmarks/manifest.csv
	@ITERATIONS=$(ITERATIONS) WARMUPS=$(WARMUPS) FIXTURE="$(FIXTURE)" FILTER="$(FILTER)" ./$(BENCH_SCRIPT) bench-vm

bench-compare: $(BIN) $(BENCH_SCRIPT) benchmarks/manifest.csv
	@ITERATIONS=$(ITERATIONS) WARMUPS=$(WARMUPS) FIXTURE="$(FIXTURE)" FILTER="$(FILTER)" ./$(BENCH_SCRIPT) bench-compare

bench-smoke: $(BIN) $(BENCH_SCRIPT) benchmarks/manifest.csv
	@ITERATIONS=1 WARMUPS=0 FIXTURE="$(FIXTURE)" FILTER="$(FILTER)" ./$(BENCH_SCRIPT) bench-smoke
