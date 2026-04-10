CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
AR = ar
ARFLAGS = rcs

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin

LIB_NAME = lima
LIB_A = $(LIB_DIR)/lib$(LIB_NAME).a

SRC_DS_DIR = src/data_structures
SRC_UI_DIR = src/ui
SRC_VFS_DIR = src/vfs
SRC_CORE_DIR = src/core
TEST_DIR = tests

# Module sources
DS_SRCS = $(wildcard $(SRC_DS_DIR)/*.c)
UI_SRCS = $(wildcard $(SRC_UI_DIR)/*.c)
VFS_SRCS = $(wildcard $(SRC_VFS_DIR)/*.c)
CORE_SRCS = $(wildcard $(SRC_CORE_DIR)/*.c)
LIB_SRCS = $(DS_SRCS) $(UI_SRCS) $(VFS_SRCS) $(CORE_SRCS)

LIB_OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))

# Tests
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/%.exe, $(TEST_SRCS))

all: $(LIB_A) $(TEST_BINS) lima

$(LIB_A): $(LIB_OBJS)
	@mkdir -p "$(LIB_DIR)"
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.exe: $(TEST_DIR)/%.c $(LIB_A)
	@mkdir -p "$(BIN_DIR)"
	$(CC) $(CFLAGS) $< -L"$(LIB_DIR)" -l$(LIB_NAME) -o $@

# Main CLI application
lima: $(LIB_A)
	@mkdir -p "$(BIN_DIR)"
	$(CC) $(CFLAGS) src/main.c -L"$(LIB_DIR)" -l$(LIB_NAME) -o $(BIN_DIR)/lima.exe

# Standalone editor demo (manual testing)
editor_demo: $(LIB_A)
	$(CC) $(CFLAGS) demos/editor_demo.c src/ui/editor.c src/ui/terminal.c \
	    src/data_structures/gap_buffer.c src/data_structures/stack.c \
	    src/data_structures/select_buffer.c \
	    -o $(BIN_DIR)/editor_demo.exe

test: all
	@for f in $(TEST_BINS); do \
		echo Running $$f...; \
		./$$f; \
	done

clean:
	rm -rf "$(BUILD_DIR)"

.PHONY: all test clean lima editor_demo
