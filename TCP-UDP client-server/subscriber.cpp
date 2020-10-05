 // Copyright Varga Raimond 2020
 #include "helpers.h"

void usage(char *file) {
	fprintf(stderr, "Usage: %s id_client server_address server_port\n", file);
	exit(0);
}

bool subscribe(string buffer) {
	string word, aux;
	stringstream it(buffer);
	it >> word;
	if(word == "subscribe") {
		it >> word;
		it >> aux;
		if(it.fail()) {
			cout << "Wrong format\n";
			return false;
			}
		cout << "subscribed ";
	} else {
		if(word == "unsubscribe") {
			it >> word;
			if(it.fail()) {
				cout << "Wrong format\n";
				return false;
			}
			cout << "unsubscribed ";
		} else {
			cout << "Wrong input.\n";
			return false;
		}
	}
	cout << word << '\n';
	return true;
}

void print_updates(char* buffer, int offset, int size) {
	struct tcp_proto_msg *msg = new struct tcp_proto_msg;
	memcpy(msg, buffer + offset, size);
	// first resolve error messages
	if (msg -> m.data_type == 5) {
		printf("%s", msg -> m.payload);
		exit(EXIT_FAILURE);
	}
	if (msg -> m.data_type > 5) {
		printf("%s\n", "Unknown data type");
		return;
	}
	struct in_addr ip;
	ip.s_addr = msg -> sender_ip;
	printf("%s:%d - %s - ", inet_ntoa(ip), ntohs(msg->sender_port), msg->m.topic);
	
	switch (msg -> m.data_type) {
		case 0: {
			printf("INT - ");
			struct payload_type0 *aux = new struct payload_type0;
			memcpy(aux, msg->m.payload, sizeof(struct payload_type0));
			if(aux -> sign == 1) {
				printf("-");
			}
			printf("%d\n", ntohl(aux -> value));
			delete aux;
			break;
			}
		case 1: {
			printf("SHORT REAL - ");
			uint16_t aux;
			memcpy(&aux, msg->m.payload, 2);
			printf("%.2f\n", ntohs(aux) / 100.);
			break;
		}
		case 2: {
			printf("FLOAT - ");
			struct payload_type2 *aux = new struct payload_type2;
			memcpy(aux, msg->m.payload, sizeof(struct payload_type2));
			if(aux -> sign == 1) {
				printf("-");
			}
			printf("%f\n", (float)(ntohl(aux -> value)) / pow(10, aux -> power));
			delete aux;
			break;
		}
		case 3: {
			printf("STRING - ");
			printf("%s\n", msg -> m.payload);
			break;
			}
		default:
			printf("\n");
		}

		delete msg;
}

void process_message_from_server(char* buffer, char* incomplete,
								short &incomplete_size, int bytes_recv) {
	short offset = 0;
	short current_size = 0;
	// check if we have any incomplete messages
	if (incomplete_size != 0) {
		// check if even the size field isn't complete
		if (incomplete_size < 2) {
			memcpy(incomplete + 1, buffer, 1);
			offset++;
			incomplete_size++;
		}
		memcpy(&current_size, incomplete, sizeof(short));
		// add the rest of the message and print it
		memcpy(incomplete + incomplete_size, buffer + offset,
			current_size - incomplete_size);
		offset += current_size - incomplete_size;
		bytes_recv -= offset;
		print_updates(incomplete, 0, current_size);
		// reset buffer for incomplete messages
		memset(incomplete, 0, BUFLEN);
		incomplete_size = 0;
	}

	if (bytes_recv >= 2) {
		memcpy(&current_size, buffer + offset, sizeof(short));
		// extract all messages
		while (current_size <= bytes_recv && bytes_recv != 0) {
			print_updates(buffer, offset, current_size);
			// update offset and take new size
			offset += current_size;
			bytes_recv -= current_size;
			// check if the next message doesn't have a complete size field
			if (bytes_recv >= 2) {
				memcpy(&current_size, buffer + offset, sizeof(short));
			} else {
				break;
			}
		}
	}

	if (bytes_recv != 0) {
		// store last incomplete message for next recv
		memcpy(incomplete, buffer + offset, bytes_recv);
		incomplete_size = bytes_recv;
	}
}

int main(int argc, char *argv[]) {
	int sockfd, n, fdmax;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN], incomplete[BUFLEN];

	if (argc < 4) {
		usage(argv[0]);
	}
	// prepare connection
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Error: socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	n = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(n == 0, "Error: inet_aton");

	n = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(n < 0, "Error: connect");

	// send id and wait to see if it's valid
	n = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "Error: send");

	fd_set read_fds, tmp_fds;		

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	memset(incomplete, 0, BUFLEN);
	short incomplete_size = 0;

	while (1) {
		tmp_fds = read_fds; 
		n = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(n < 0, "Error: select");

		if (FD_ISSET(0, &tmp_fds)) {
	  		// read command
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}			
			if(subscribe(buffer)) {
				// send message to server
				n = send(sockfd, buffer, strlen(buffer), 0);
				DIE(n < 0, "Error: send");
			}
		}
		if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, BUFLEN, 0);
			DIE(n < 0, "Error: receive");
			// server closed forcefully
			if (n == 0) {
				break;
			}
			process_message_from_server(buffer, incomplete,
				incomplete_size, n);
		}
	}

	close(sockfd);

	return 0;
}