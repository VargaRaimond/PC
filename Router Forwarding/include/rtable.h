#include "trie.h"

void read_rtable(struct TrieNode **root);
struct route_table_entry *get_best_route(__u32 dest_ip, struct TrieNode *root);
