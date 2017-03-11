CC=gcc
CFLAGS= -ansi -pedantic -Wall -lpthread -g

client: client.o client.o
     $(CC) $(CFLAGS) -o client client.o client.o