CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11 -Iinclude
LDFLAGS := 

BIN_DIR := bin
BIN     := $(BIN_DIR)/xor

SRC := \
  src/util.c \
  src/xor_main.c

OBJ := $(SRC:.c=.o)

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	@echo "✔ Build OK → $@"

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	$(BIN) -h

clean:
	rm -rf $(BIN_DIR) $(OBJ)
