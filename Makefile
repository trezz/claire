# Claire Project Makefile

# Compiler and flags
CC = gcc
CFLAGS = -ansi -pedantic -Wall -Wextra -O2 -I.
DEBUG_CFLAGS = -ansi -pedantic -Wall -Wextra -g -O0 -DDEBUG -I.
LDFLAGS =

# Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib

# Source files
HASH_SRCS = hash.c
MAP_SRCS = map.c
ALL_SRCS = $(HASH_SRCS) $(MAP_SRCS)

# Object files
HASH_OBJS = $(HASH_SRCS:%.c=$(OBJ_DIR)/%.o)
MAP_OBJS = $(MAP_SRCS:%.c=$(OBJ_DIR)/%.o)
ALL_OBJS = $(ALL_SRCS:%.c=$(OBJ_DIR)/%.o)

# Test files
TEST_BINS = $(BIN_DIR)/hash_test $(BIN_DIR)/map_test

# Library files
LIBNAME = libclaire
STATIC_LIB = $(LIB_DIR)/$(LIBNAME).a
DEBUG_STATIC_LIB = $(LIB_DIR)/$(LIBNAME)-debug.a

# Default target
.PHONY: all
all: $(ALL_OBJS) $(STATIC_LIB)

# Object files compilation
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug object files
$(OBJ_DIR)/debug/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

# Static library creation
$(STATIC_LIB): $(ALL_OBJS) | $(LIB_DIR)
	ar rcs $@ $^

# Debug static library creation
$(DEBUG_STATIC_LIB): $(ALL_OBJS:$(OBJ_DIR)/%.o=$(OBJ_DIR)/debug/%.o) | $(LIB_DIR)
	ar rcs $@ $^

# Test compilation
$(BIN_DIR)/hash_test: hash_test.c $(OBJ_DIR)/hash.o | $(BIN_DIR)
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

$(BIN_DIR)/map_test: map_test.c $(OBJ_DIR)/map.o $(OBJ_DIR)/hash.o | $(BIN_DIR)
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR): | $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)

$(BIN_DIR): | $(BUILD_DIR)
	mkdir -p $(BIN_DIR)

$(LIB_DIR): | $(BUILD_DIR)
	mkdir -p $(LIB_DIR)

# Debug build
.PHONY: debug
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: $(ALL_OBJS:$(OBJ_DIR)/%.o=$(OBJ_DIR)/debug/%.o) $(DEBUG_STATIC_LIB)

# Library targets
.PHONY: lib lib-debug
lib: $(STATIC_LIB)
lib-debug: $(DEBUG_STATIC_LIB)

# Test targets
.PHONY: test
test: $(BIN_DIR)/hash_test $(BIN_DIR)/map_test
	@echo "Running hash tests..."
	@$(BIN_DIR)/hash_test
	@echo "Running map tests..."
	@$(BIN_DIR)/map_test

.PHONY: test-hash
test-hash: $(BIN_DIR)/hash_test
	@echo "Running hash tests..."
	@$(BIN_DIR)/hash_test

.PHONY: test-map
test-map: $(BIN_DIR)/map_test
	@echo "Running map tests..."
	@$(BIN_DIR)/map_test

# Clean targets
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: distclean
distclean: clean

# Help target
.PHONY: help
help:
	@echo "Claire Project Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build all object files and production library (default)"
	@echo "  debug      - Build debug object files and debug library"
	@echo "  lib        - Build production static library"
	@echo "  lib-debug  - Build debug static library"
	@echo "  test       - Build and run all tests"
	@echo "  test-hash  - Build and run hash tests only"
	@echo "  test-map   - Build and run map tests only"
	@echo "  hash       - Build hash module object file"
	@echo "  map        - Build map module object file"
	@echo "  clean      - Remove build directory"
	@echo "  distclean  - Alias for clean"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CC         - C compiler (default: gcc)"
	@echo "  CFLAGS     - Compiler flags"

# Pattern-based targets for individual modules
.PHONY: hash map
hash: $(OBJ_DIR)/hash.o
map: $(OBJ_DIR)/map.o $(OBJ_DIR)/hash.o

# Prevent deletion of intermediate files
.PRECIOUS: $(OBJ_DIR)/%.o $(OBJ_DIR)/debug/%.o
