CC=gcc
CFLAGS= -fPIC -Wall -pedantic

server: server.cpp
	g++ -Wall -g -pedantic server.cpp -o server.o


client: client.cpp
	g++ -Wall -g -pedantic client.cpp -o client.o

cserver:
		$(CC) $(CFLAGS) -g server.c utils.c -o cserver.o

