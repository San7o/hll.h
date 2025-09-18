## --- Settings ---

CFLAGS=-Wall -Werror -Wpedantic -ggdb -std=c99 -Wno-initializer-overrides
LDFLAGS=-lm
CC=gcc

OUT_NAME=hll
OBJ=main.o

## --- Commands ---

# --- Targets ---

all: $(OUT_NAME)

run: $(OUT_NAME)
	chmod +x $(OUT_NAME)
	./$(OUT_NAME)

$(OUT_NAME): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(CLAGS) -o $(OUT_NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJ) 2>/dev/null || :

distclean:
	rm $(OUT_NAME) 2>/dev/null || :
