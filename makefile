CC = gcc
CFLAGS = -g -Wall -Werror
HEADERS = -pthread

all: server client

server: Server.c
	$(CC) $(CFLAGS) $(HEADERS) -o server Server.c -ljson-c

client: Client.c
	$(CC) $(CFLAGS) $(HEADERS) -o client Client.c -ljson-c

clean:
	rm -f server client

fresh:
	clean all
