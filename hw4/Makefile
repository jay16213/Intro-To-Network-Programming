# Makefile
CC = gcc

main: server.c client.c util.c headers.h
	${CC} server.c util.c -o server
	${CC} client.c util.c -o client
.PHONY: clean
clean:
	rm -rf server client
