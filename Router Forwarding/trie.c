#include "trie.h" 
  
  
// Returns new trie node (initialized to NULLs) 
struct TrieNode *getNode(void) { 
    struct TrieNode *newNode = NULL; 
    newNode = (struct TrieNode *)malloc(sizeof(struct TrieNode)); 
    if (newNode) { 
        int i; 
        newNode->isEndOfWord = false; 
        for (i = 0; i < BINARY_SIZE; i++) 
            newNode->children[i] = NULL; 
    } 
    newNode -> value = NULL;
  
    return newNode; 
} 
  
void insert(struct TrieNode *root, unsigned int key, struct route_table_entry *value) { 
    int offset = 0;
    
    // get mask offset by shifting bits
    while((ntohl(value -> mask) & (1 << offset)) == 0) {
        offset++;
    }

    unsigned int index; 
    struct TrieNode *iter = root;
  
    for (int level = IP_SIZE; level >= offset; level--) {  
        index = 1 << level;
        index &= key;
        index = index >> level;
        if (!iter->children[index]) 
            iter->children[index] = getNode(); 
  
        iter = iter->children[index]; 
    } 
  
    // mark last node as leaf and add all values
    iter->isEndOfWord = true; 
    iter -> value = (struct route_table_entry *)malloc(sizeof(struct route_table_entry));
    memcpy(iter-> value, value, sizeof(struct route_table_entry));
} 
  
// Returns true if key presents in trie, else false 
struct route_table_entry* search(struct TrieNode *root, unsigned int key) { 
    int length = 31; 
    unsigned int index; 
    struct TrieNode *iter = root; 
    struct route_table_entry* result = NULL;
  
    for (int level = length; level >= 0; level--) { 
        // extract every bit to compare with our prefixes
        index = 1 << level;
        index = key & index; 
        index = index >> level;
        // if we find a match update result
        if(iter->isEndOfWord) {
            result = iter -> value;
        }
        if (!iter->children[index]) 
            return result; 
  
        iter = iter->children[index]; 
    } 
  
    return result; 
} 

void free_all(struct TrieNode* root) {
    if(!root) return;
    // recursive free
    for (int i = 0; i < BINARY_SIZE; i++)
       free_all(root->children[i]);
    // base case
    free(root);
}