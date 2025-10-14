# SPDX-License-Identifier: MIT
# Author:  Giovanni Santini
# Mail:    giovanni.santini@proton.me
# Github:  @San7o

#
# Compiler flags
#
CFLAGS=-Wall -Werror -Wpedantic -ggdb -std=c99 -Wno-initializer-overrides
LDFLAGS=-lm
CC=gcc

#
# Project files
#
OUT_NAME=example
OBJ=example.o

#
# Commands
#
all: $(OUT_NAME)

run: $(OUT_NAME)
	chmod +x $(OUT_NAME)
	./$(OUT_NAME)

clean:
	rm -f $(OBJ)

distclean:
	rm -f $(OUT_NAME)

$(OUT_NAME): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(CFLAGS) -o $(OUT_NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
