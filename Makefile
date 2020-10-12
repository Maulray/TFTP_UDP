CC = gcc
FLAGS = -Wall -Wextra -g #-fsanitize=address

all : prep server client clean
	chmod -R 700 ./*
	clear

prep :
	rm output.txt

server : server.o
	$(CC) $(FLAGS) $^ -o out/server

server.o : src/server.c
	$(CC) $(FLAGS) $^ -c

client : client.o
	$(CC) $(FLAGS) $^ -o out/client

client.o : src/client.c
	$(CC) $(FLAGS) $^ -c

run_server :
	./out/server 127.0.0.1 8080

run_client :
	./out/client 127.0.0.1 8080 "data/test.txt"

clean :
	rm -rf *.o
