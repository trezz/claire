# Claire Project Makefile

# Compiler and flags
CC = gcc
CFLAGS = -ansi -pedantic -Wall -Wextra -O2 -I.
DEBUG_CFLAGS = -ansi -pedantic -Wall -Wextra -g -O0 -DDEBUG -I.

# Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib

# Source files
SRCS = hash.c map.c
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)
DEBUG_OBJS = $(SRCS:%.c=$(OBJ_DIR)/debug/%.o)

# Libraries
LIBNAME = libclaire
STATIC_LIB = $(LIB_DIR)/$(LIBNAME).a
DEBUG_LIB = $(LIB_DIR)/$(LIBNAME)-debug.a

# Default target
.PHONY: all
all: $(STATIC_LIB)

# Libraries
$(STATIC_LIB): $(OBJS) | $(LIB_DIR)
	ar rcs $@ $^

$(DEBUG_LIB): $(DEBUG_OBJS) | $(LIB_DIR)
	ar rcs $@ $^

# Object files
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/debug/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

# Test binaries (mode depends on TEST_MODE variable)
TEST_MODE ?= prod
ifeq ($(TEST_MODE),debug)
    TEST_CFLAGS = $(DEBUG_CFLAGS)
    TEST_HASH_DEPS = $(OBJ_DIR)/debug/hash.o
    TEST_MAP_DEPS = $(OBJ_DIR)/debug/map.o $(OBJ_DIR)/debug/hash.o
else
    TEST_CFLAGS = $(CFLAGS)
    TEST_HASH_DEPS = $(OBJ_DIR)/hash.o
    TEST_MAP_DEPS = $(OBJ_DIR)/map.o $(OBJ_DIR)/hash.o
endif

$(BIN_DIR)/hash_test: hash_test.c $(TEST_HASH_DEPS) | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) $^ -o $@

$(BIN_DIR)/map_test: map_test.c $(TEST_MAP_DEPS) | $(BIN_DIR)
	$(CC) $(TEST_CFLAGS) $^ -o $@

# Directories
$(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR):
	mkdir -p $@

# High-level targets
.PHONY: lib lib-debug debug
lib: $(STATIC_LIB)
lib-debug: $(DEBUG_LIB)
debug: $(DEBUG_LIB)

# Test targets
.PHONY: test test-debug
test: $(BIN_DIR)/hash_test $(BIN_DIR)/map_test
	@echo "Running hash tests..."
	@$(BIN_DIR)/hash_test
	@echo "Running map tests..."
	@$(BIN_DIR)/map_test

test-debug: clean-tests
	@$(MAKE) TEST_MODE=debug $(BIN_DIR)/hash_test $(BIN_DIR)/map_test
	@echo "Running hash tests (debug)..."
	@$(BIN_DIR)/hash_test
	@echo "Running map tests (debug)..."
	@$(BIN_DIR)/map_test

# Individual test targets
.PHONY: hash_test map_test vec_test hash_test-debug map_test-debug
hash_test: $(BIN_DIR)/hash_test
	@echo "Running hash tests..."
	@$(BIN_DIR)/hash_test

map_test: $(BIN_DIR)/map_test
	@echo "Running map tests..."
	@$(BIN_DIR)/map_test

hash_test-debug: clean-tests
	@$(MAKE) TEST_MODE=debug $(BIN_DIR)/hash_test
	@echo "Running hash tests (debug)..."
	@$(BIN_DIR)/hash_test

map_test-debug: clean-tests
	@$(MAKE) TEST_MODE=debug $(BIN_DIR)/map_test
	@echo "Running map tests (debug)..."
	@$(BIN_DIR)/map_test

# Module targets
.PHONY: hash map
hash: $(OBJ_DIR)/hash.o
map: $(OBJ_DIR)/map.o $(OBJ_DIR)/hash.o

# Clean targets
.PHONY: clean clean-tests
clean:
	rm -rf $(BUILD_DIR)

clean-tests:
	rm -f $(BIN_DIR)/hash_test $(BIN_DIR)/map_test

# Help
.PHONY: help
help:
	@echo "Claire Project Makefile"
	@echo ""
	@echo "Main targets:"
	@echo "  all          - Build production library (default)"
	@echo "  lib          - Build production library"
	@echo "  lib-debug    - Build debug library"
	@echo "  test         - Build and run all tests (production)"
	@echo "  test-debug   - Build and run all tests (debug)"
	@echo ""
	@echo "Individual tests:"
	@echo "  hash_test, map_test - Run specific test (production)"
	@echo "  hash_test-debug, map_test-debug - Debug versions"
	@echo ""
	@echo "Modules:"
	@echo "  hash, map - Build individual modules"
	@echo ""
	@echo "Utilities:"
	@echo "  clean        - Remove all build files"
	@echo "  help         - Show this help"

.PRECIOUS: $(OBJ_DIR)/%.o $(OBJ_DIR)/debug/%.o
