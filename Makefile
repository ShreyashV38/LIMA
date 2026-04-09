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
SRC_VFS_DIR = src/vfs
SRC_CORE_DIR = src/core
TEST_DIR = tests

# Module sources
DS_SRCS = $(wildcard $(SRC_DS_DIR)/*.c)
VFS_SRCS = $(wildcard $(SRC_VFS_DIR)/*.c)
CORE_SRCS = $(wildcard $(SRC_CORE_DIR)/*.c)
LIB_SRCS = $(DS_SRCS) $(VFS_SRCS) $(CORE_SRCS)

LIB_OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))

# Tests
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/%.exe, $(TEST_SRCS))

all: $(LIB_A) $(TEST_BINS)

$(LIB_A): $(LIB_OBJS)
	@if not exist "$(subst /,\,$(LIB_DIR))" mkdir "$(subst /,\,$(LIB_DIR))"
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: src/%.c
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.exe: $(TEST_DIR)/%.c $(LIB_A)
	@if not exist "$(subst /,\,$(BIN_DIR))" mkdir "$(subst /,\,$(BIN_DIR))"
	$(CC) $(CFLAGS) $< -L"$(LIB_DIR)" -l$(LIB_NAME) -o $@

test: all
	@for f in $(TEST_BINS); do \
		echo Running $$f...; \
		./$$f; \
	done

clean:
	rm -rf "$(BUILD_DIR)"

.PHONY: all test clean
