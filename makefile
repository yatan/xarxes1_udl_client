CC=gcc
CFLAGS= -ansi -pedantic -Wall -lpthread

client:
				$(CC) client.c -o client $(CFLAGS)
