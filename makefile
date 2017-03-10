CC=gcc
CFLAGS= -ansi -pedantic -Wall

client: client.o client.o
     $(CC) $(CFLAGS) -o client client.o client.o