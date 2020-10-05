// Copyright Varga Raimond 2020
#include "helpers.h"

void send_stored_msg(client &subscriber) {
	// iterate through stored messages and send them
	char buffer[BUFLEN];
	memset(buffer, 0, BUFLEN);
	for(auto message : subscriber.stored_msg) {
		memcpy(buffer, &message, sizeof(struct tcp_proto_msg));
		int n = send(subscriber.sock, buffer, message.size, 0);
		DIE(n < 0, "Error: send");
	}
	subscriber.stored_msg.clear();
}

void update_message_size(struct tcp_proto_msg *msg) {
	// all messages have basic fields: size, ip ,port, topic and data_type
	msg -> size = MESSAGE_BASE_SIZE;
	switch (msg -> m.data_type) {
		case 0: {
			msg -> size += sizeof(struct payload_type0);
			break;
			}
		case 1: {
			// only a short int
			msg -> size += 2;
			break;
		}
		case 2: {
			msg -> size += sizeof(struct payload_type2);
			break;
		}
		default: {
			msg -> size += strlen(msg -> m.payload);
			break;
			}
	}
}


// check if an id is valid and adds new clients to our database
bool add_valid_client(vector<client> &clients, string id, int socket) {
	for (auto it = clients.begin(); it != clients.end(); it++) {
		if (it -> id == id) {
			if (it -> is_online) {
				struct tcp_proto_msg *err_msg = new struct tcp_proto_msg;
				memset(err_msg, 0, sizeof(tcp_proto_msg));
				err_msg->m.data_type = 5;
				sprintf(err_msg->m.payload, "Id already in use.\n");
				update_message_size(err_msg);
				int n = send(socket, err_msg, err_msg -> size, 0);
				// free memory
				delete err_msg;
				return false;
			} else {
				/* if a client returns, update his information and send stored 
				messages */
				it -> is_online = true;
				it -> sock = socket;
				send_stored_msg(*it);
				return true;
			}
		}
	}
	// we didn't find the id so we have a new client
	clients.push_back(client(socket,id));
	return true;
}

void disconnect_client(vector<client> &clients, int socket) {
	for (auto it = clients.begin(); it != clients.end(); it++) {
		if (it -> sock == socket) {
			it -> is_online = false;
			printf("Client %s disconnected.\n", it -> id.c_str());
			return;
		}
	}
}

void update_subscription(vector<client> &clients, int socket, string buffer) {
	string type, topic, sf;
	stringstream string_it(buffer);
	string_it >> type >> topic;

	// find client and update his status on current topic
	for (auto it = clients.begin(); it != clients.end(); it++) {
		// check format in case there was an error in receiving the message
		if (it -> sock == socket) {
			if (type == "subscribe") {
				string_it >> sf;
				if(string_it.fail()) {
					cout << "Wrong format from " << it -> id << '\n';
					return;
				}
				it -> sub(topic, stoi(sf) == 1? true:false);
				return;
			}
			if (type == "unsubscribe") {
				it -> unsub(topic);
				return;
			}
			cout << "Command " << type << " doesn't exist.\n"; 
			return;
		}
	}
}

void notify_subscribers(struct tcp_proto_msg *msg, vector<client> &clients) {
	pair<string, bool> subscribtion;

	for (auto it = clients.begin(); it != clients.end(); it++) {
		if(it -> has_sub(msg -> m.topic, subscribtion)) {
			if(it -> is_online) {
				int n = send(it -> sock, msg, msg -> size, 0);
				DIE(n < 0, "send");
			} else {
				/* store messages for offline clients that have sf 1 for a
				subscription */
				if (subscribtion.second) {
					it -> stored_msg.push_back(*msg);
				}
			}
		}
	}
}

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int prepare_connection(struct sockaddr_in &addr, int &portno, int &sock_udp) {
	int ret, sock_tcp;
	// get tcp socket
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "Error: socket tcp");

	// get udp socket
	sock_udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "Error: socket udp");

	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portno);
	addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock_tcp, (struct sockaddr *) &addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Error: bind tcp");

	ret = bind(sock_udp, (struct sockaddr *) &addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Error: bind udp");

	ret = listen(sock_tcp, MAX_CLIENTS);
	DIE(ret < 0, "Error: listen");
	return sock_tcp;
}

void process_tcp_client(vector<client>& client_ids, int& fdmax,
						fd_set& read_fds, int sock_tcp) {
	int newsock_tcp, ret;
	char buffer[BUFLEN];
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	// accept new connection
	clilen = sizeof(cli_addr);
	newsock_tcp = accept(sock_tcp, (struct sockaddr *) &cli_addr, &clilen);
	int flag = 1;
	// deactivate neagle
	setsockopt(newsock_tcp, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
	DIE(newsock_tcp < 0, "Error: accept");

	// add new socket
	FD_SET(newsock_tcp, &read_fds);
	if (newsock_tcp > fdmax) { 
		fdmax = newsock_tcp;
	}

	// after we get a new client we immediately wait for his id
	memset(buffer, 0, BUFLEN);
	ret = recv(newsock_tcp, buffer, sizeof(buffer), 0);
	DIE(ret < 0, "Error: receive from tcp");

	// add client or update his status to online if his not new
	if (!add_valid_client(client_ids, buffer, newsock_tcp)) {
		// refuse connection if we already have a client with this id
		close(newsock_tcp);
		FD_CLR(newsock_tcp, &read_fds);
	} else {
		// client successfuly connected
		printf("New client %s connected from %s:%d.\n", buffer,
				inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	}
}

void check_active_sockets(vector<client> &client_ids, int& fdmax, 
				fd_set& read_fds, fd_set& tmp_fds, int sock_tcp, int sock_udp) {
	int ret;
	char buffer[BUFLEN];
	struct sockaddr_in cli_addr;
	socklen_t len = sizeof(cli_addr);
	memset(&cli_addr, 0, sizeof(struct sockaddr_in));

	// iterate through all sockets
	for (int i = 1; i <= fdmax; i++) {
		if (FD_ISSET(i, &tmp_fds)) {
			if (i == sock_tcp) {
				process_tcp_client(client_ids, fdmax, read_fds, sock_tcp);
				continue;
			}
			if (i == sock_udp) {
				// create new tcp message
				struct tcp_proto_msg *msg = new tcp_proto_msg();
				memset(buffer, 0, BUFLEN);
				ret = recvfrom(sock_udp, buffer, BUFLEN, 0, 
					(struct sockaddr *)(&cli_addr), &len);

				// add udp ip and port to message structure
				msg -> sender_port = cli_addr.sin_port;
				msg -> sender_ip = cli_addr.sin_addr.s_addr;
				// add the buffer in our structure and update size
				memcpy(&(msg->m), buffer, sizeof(buffer));
				update_message_size(msg);
				// check if we have subscribers and send them this message
				notify_subscribers(msg, client_ids);

				delete msg;
			} else {
				// we received a message from a client
				memset(buffer, 0, BUFLEN);
				ret = recv(i, buffer, sizeof(buffer), 0);
				DIE(ret < 0, "Error: receive");

				// check if client disconnected
				if (ret == 0) {
					disconnect_client(client_ids, i);
					close(i);
					FD_CLR(i, &read_fds);
				} else {
					update_subscription(client_ids, i, buffer);

				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int sock_tcp, portno, sock_udp, ret;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	fd_set read_fds, tmp_fds;
	int fdmax;
	vector<client> client_ids;

	if (argc < 2) {
		usage(argv[0]);
	}
	portno = atoi(argv[1]);
	DIE(portno == 0, "Wrong arguments.");
	sock_tcp = prepare_connection(serv_addr, portno, sock_udp);

	// add new sockets for both connections and stdin
	FD_SET(0, &read_fds);
	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
	fdmax = max(sock_udp, sock_tcp);

	while (1) {
		tmp_fds = read_fds; 	
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Error: select");

		// check if we got the command to close the server
		if (FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN, stdin);
			
			if (strncmp(buffer, "exit", 4) == 0) {
				for(auto client : client_ids) {
					if(client.is_online) {
						close(client.sock);
					}
				}
				break;
			}			
		}
		// check all sockets and process everything
		check_active_sockets(client_ids, fdmax, read_fds, 
					tmp_fds, sock_tcp, sock_udp);
	}

	close(sock_tcp);
	close(sock_udp);

	return 0;
}
