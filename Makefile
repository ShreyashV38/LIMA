CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
SRC_DS_DIR = src/data_structures
SRC_VFS_DIR = src/vfs
TEST_DIR = tests
OBJ_DIR = .
BIN_DIR = .

# Sources and Tests
DS_SRCS = $(wildcard $(SRC_DS_DIR)/*.c) $(wildcard $(SRC_VFS_DIR)/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

# Binary targets
TEST_BINS = $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/%.exe, $(TEST_SRCS))

all: $(TEST_BINS)

$(BIN_DIR)/%.exe: $(TEST_DIR)/%.c $(DS_SRCS)
	$(CC) $(CFLAGS) $^ -o $@

test: all
	@for f in $(TEST_BINS); do \
		echo Running $$f...; \
		./$$f; \
	done

clean:
	rm -f *.exe *.o

.PHONY: all test clean
