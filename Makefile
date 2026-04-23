CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lncurses

pong: pong.c
	$(CC) $(CFLAGS) pong.c -o pong $(LIBS)

clean:
	rm -f pong