# Copyright PC team

CFLAGS = -Wall -g

# Server port
PORT = 

# Server ip adress
IP_SERVER = 

all: server subscriber

# Compile server.cpp
server: server.cpp
	g++ server.cpp -o server

# Compile subscriber.cpp
subscriber: subscriber.cpp
	g++ subscriber.cpp -o subscriber

.PHONY: clean run_server run_client

run_server:
	./server ${PORT}


run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
