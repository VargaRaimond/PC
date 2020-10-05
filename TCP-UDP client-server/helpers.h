// Copyright Varga Raimond 2020
#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/stdc++.h> 
#include <netinet/tcp.h>
using namespace std;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1561	// max dimension for a message
#define MAX_CLIENTS	5	// i use select so anything bigger than 1 works
#define MESSAGE_BASE_SIZE 61

// structure for the udp message
struct message {
	char topic[50];
	unsigned char data_type;
	char payload[1500];
}__attribute__((packed));

// structure for my tcp protocol
struct tcp_proto_msg {
	short size;
	int sender_port;
	int sender_ip;
	struct message m;
}__attribute__((packed));

// structures for the more complicated messages
struct payload_type0 {
	unsigned char sign;
	uint32_t value;
}__attribute__((packed));

struct payload_type2 {
	unsigned char sign;
	uint32_t value;
	uint8_t power;
}__attribute__((packed));


class client {
public:
	int sock;
	string id;
	vector<pair<string, bool>> subs;
	bool is_online;
	vector<tcp_proto_msg> stored_msg;

	client(int sock, string id) {
		this->sock = sock;
		is_online = true;
		this->id.assign(id);
	}

	void sub(string sub_topic, bool sf) {
		subs.push_back(pair<string, bool>(sub_topic, sf));
	}

	void unsub(string sub_topic) {
		for (auto it = subs.begin(); it != subs.end(); it++) {
			if (it -> first == sub_topic) {
				subs.erase(it);
				break;
			}
		}
	}

	bool has_sub(string sub_topic, pair<string, bool> &subscription) {
		// search for a subscription by topic and return it
		for (auto it : subs) {
			if (it.first == sub_topic) {
				subscription = it;
				return true;
			}
		}
		return false;
	}
};

#endif