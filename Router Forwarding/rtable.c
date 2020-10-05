#include "rtable.h"

void read_rtable(struct TrieNode **root) {
	char elem[30];
	FILE* ptr = fopen("rtable.txt", "r");
	if(ptr == NULL) {
		printf("Error! Cannot open rtable file");   
      	exit(1);
	}

	struct route_table_entry* reader = (struct route_table_entry*)malloc(sizeof(struct route_table_entry));
	// read a group of 4 every while step
	while(fscanf(ptr,"%s", elem) != EOF) {
		// prefix
		reader -> prefix = inet_addr(elem);

		// next_hop
		fscanf(ptr,"%s", elem);
		reader -> next_hop = inet_addr(elem);

		// mask
		fscanf(ptr,"%s", elem);
		reader -> mask = inet_addr(elem);

		// interface
		fscanf(ptr,"%s", elem);
		reader -> interface = atoi(elem);
		insert(*root, ntohl(reader-> prefix), reader);
	}
	fclose(ptr);
}

struct route_table_entry *get_best_route(__u32 dest_ip, struct TrieNode *root) {
	return search(root, dest_ip);
} 
