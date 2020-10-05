#include <stdbool.h> 
#include "skel.h"

#define BINARY_SIZE (2) 
#define IP_SIZE (31)

struct TrieNode 
{ 
    struct TrieNode *children[BINARY_SIZE]; 
    struct route_table_entry *value;
    bool isEndOfWord; 
}; 
struct TrieNode *getNode(void) ;
void insert(struct TrieNode *root, unsigned int key, struct route_table_entry *value) ;
struct route_table_entry* search(struct TrieNode *root, unsigned int key);
void free_all(struct TrieNode* root);