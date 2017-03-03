CC=gcc
CFLAGS=-I.

client: client.o client.o
     $(CC) -o client client.o client.o -I.