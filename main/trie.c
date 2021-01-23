#include "trie.h"
#include "code.h"
#include <inttypes.h>
#include <stdlib.h>

//Creates the trie_node. Returns trie after allocating memroy
//
//code: index passed
TrieNode *trie_node_create(uint16_t code) {
  TrieNode *trie = (TrieNode *)malloc(sizeof(TrieNode));
  if (!trie) {
    return 0;
  }
  trie->code = code;
  //Sets all children to NULL just in case
  for (uint32_t i = 0; i < ALPHABET; i++) {
    trie->children[i] = NULL;
  }
  return trie;
}

//Frees the memeory for TrieNode
//
//n: node passed
void trie_node_delete(TrieNode *n) {
  free(n);
  return;
}

//Initializes a Trie: with the code EMPTY_CODE
//
TrieNode *trie_create(void) {
  TrieNode *trie = trie_node_create(EMPTY_CODE);
  return trie;
}

//Frees all children and nodes except for root
//
//root: starting node
void trie_reset(TrieNode *root) {
  for (uint32_t i = 0; i < ALPHABET; i++) {
    if (root->children[i]) {
      trie_delete(root->children[i]);
    }
  }
  return;
}

//Uses recursion to delete all children of children
//
//n: trie node to recurse
void trie_delete(TrieNode *n) {
  for (uint32_t i = 0; i < ALPHABET; i++) {
    if (n->children[i]) {
      //Calls itself to check if the children have any children
      trie_delete(n->children[i]);
    }
  }
  trie_node_delete(n);
  return;
}

//Returns children at sym
//
//n: node that is being checked
//sym: sym of children being searched
TrieNode *trie_step(TrieNode *n, uint8_t sym) {
  return n->children[sym];
}
