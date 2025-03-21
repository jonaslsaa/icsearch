CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
LDFLAGS = -lm

SRC_DIR = src
OBJ_DIR = obj

MAIN_SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/ic_runtime.c $(SRC_DIR)/ic_search.c
TEST_SRCS = $(SRC_DIR)/main_test.c $(SRC_DIR)/ic_runtime.c $(SRC_DIR)/ic_search.c

MAIN_OBJS = $(MAIN_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = main
TEST = test

.PHONY: all clean test

all: $(TARGET) $(TEST)

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Main program
$(TARGET): $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Test program
$(TEST): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Run the test suite
test: $(TEST)
	./$(TEST)

# Cleanup
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST)