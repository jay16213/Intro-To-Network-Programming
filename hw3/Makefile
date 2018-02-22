# Makefile
CC = gcc

main: sender1.c sender2.c sender3.c receiver1.c receiver2.c receiver3.c util.c headers.h
	${CC} sender1.c util.c -o sender1
	${CC} sender2.c util.c -o sender2
	${CC} sender3.c util.c -o sender3
	${CC} receiver1.c util.c -o receiver1
	${CC} receiver2.c util.c -o receiver2
	${CC} receiver3.c util.c -o receiver3
.PHONY: clean
clean:
	rm -rf sender1 sender2 sender3 receiver1 receiver2 receiver3
