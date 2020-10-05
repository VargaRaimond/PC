 
			Homework 2 PC

	Group: 325CA
	Name: Raimond Varga
	Programming language: C++


		Functionality

	I implemented a server that works with both tcp and udp clients, by making
	use of the select() function, and a client that also uses the select function
	to receive messages from stdin or the server. The main idea is that the client
	subscribes to topics and then receives, through the servers, every update from
	those certain topics. It also has a store&forward functionality, that the
	clients can choose to use. I added comments in the code that explains every
	detail of my implementation.
	I tested my program in multiple ways - a lot of clients, thousand of stored
	messages, sending with no delay, etc. The only problem that might come up
	is trying to store too many messages, because I store them internally so
	it will use RAM memory.

		General Structure

	I organised my project in 3 sources:
	- server.cpp;
	- subscriber.cpp;
	- helpers.h which has all libraries includes, all defines and also all
	structures: for messages and a class for a client;

		TCP protocol

	I created structures for the tcp messages. I receive an udp message in
	another structure, I prepare my tcp struct with all information, and
	send it to the client, which also extracts all possible message structures
	from char buffers.
	To be efficient I only sent the messages with their size calculated, so I
	saved a lot of space by not sending the max size of buffer everytime - example
	sending 1561 bytes for a message that is only 60 bytes long.
	By doing this, I encountered a problem, tcp concatenates or cuts messages to 
	be more efficient. I solved this in client, with the help of my tcp structure:
	I knew every size of every message so I extracted them from the buffer, and
	stored the incomplete ones to finish them when I receive the next buffer.

		Error management

	I tried to take into account most errors that came to mind:
	1. Two clients with same id:
	- the second one gets denied his connection receiving a message in which
	he is informed by the situation;
	2. Wrong format message from subscriber:
	- the client verifies the format and only sends it if it's not wrong,
	printing a "Wrong input" message otherwise;
	- to be safe, I also verify the format in the server and print if the format
	is wrong;
	3. Most errors from standard functions:
	- for these I used DIE calls because the program couldn't continue from that
	state and I didn't want it to get in an unknown or error state;
	4. Forcefully closing the server:
	- I also closed all clients sockets that are active so they don't get a seg fault;
	5. Usage for the executables:
	- I used the function from PC team which prints the corect format and exits the
	program if the parameters given are wrong;

		Documentation

	I mostly used my sources from tcp and udp practical courses.
	https://ocw.cs.pub.ro/courses/pc/laboratoare/06
	https://ocw.cs.pub.ro/courses/pc/laboratoare/08
	I also used the forum for this homework.