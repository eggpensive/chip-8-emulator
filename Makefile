CFLAGS = -std=c17 -Wall -Wextra -Werror
LDFLAGS = -lmingw32 -lSDL2main -lSDL2

chip8: chip8.c
	gcc chip8.c -o chip8 $(CFLAGS) $(LDFLAGS)