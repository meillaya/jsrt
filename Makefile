CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Werror -pedantic -Iinclude -g
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN := $(BUILD_DIR)/jsrt
TEST_BIN := $(BUILD_DIR)/test_jsrt

SRC := src/main.c src/memory.c src/diagnostics.c src/lexer.c src/ast.c src/parser.c src/value.c src/env.c src/gc.c src/eval.c
LIB_SRC := src/memory.c src/diagnostics.c src/lexer.c src/ast.c src/parser.c src/value.c src/env.c src/gc.c src/eval.c
TEST_SRC := tests/test_runner.c tests/test_lexer.c tests/test_parser.c tests/test_eval.c tests/test_diagnostics.c

OBJ := $(SRC:src/%.c=$(OBJ_DIR)/src/%.o)
TEST_OBJ := $(LIB_SRC:src/%.c=$(OBJ_DIR)/src/%.o) $(TEST_SRC:tests/%.c=$(OBJ_DIR)/tests/%.o)

.PHONY: all clean test debug

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

clean:
	rm -rf $(BUILD_DIR)
