#include "code.h"
#include "io.h"
#include "trie.h"
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BYTE 8;
#define BLOCK 4096;

//These extern keep track of the stats for the bits

extern uint64_t uncompressed_bits;
extern uint64_t compressed_bits;

uint8_t bitlength(uint16_t next_code);

int main(int argc, char **argv) {
  int c;
  bool stats = false;
  bool infile_bol = false;
  bool outfile_bol = false;
  char *infile_name;
  char *outfile_name;
  int infile = 0;
  int outfile = 1;
  while ((c = getopt(argc, argv, "vi:o:")) != -1) {
    switch (c) {
    case 'v':
      stats = true;
      break;
    case 'i':
      infile_bol = true;
      infile_name = optarg;
      break;
    case 'o':
      outfile_bol = true;
      outfile_name = optarg;
      break;
    }
  }
  //Opened outside of the the getopt to avoid make infer error
  if (infile_bol) {
    infile = open(infile_name, O_RDONLY);
    if (infile == -1) {
      puts("Error opening infile");
      return 0;
    }
  }
  if (outfile_bol) {
    outfile = open(outfile_name, O_WRONLY | O_CREAT | O_TRUNC);
    if (outfile == -1) {
      close(infile);
      puts("Error opening outfile");
      return 0;
    }
  }
  FileHeader hd = { 0, 0 };
  hd.magic = 0x8badbeef;
  struct stat header_protection;
  fstat(infile, &header_protection);
  hd.protection = header_protection.st_mode;
  fchmod(outfile, hd.protection);
  write_header(outfile, &hd);

  TrieNode *root = trie_create();
  if (!root) {
    close(infile);
    close(outfile);
    return 0;
  }
  TrieNode *curr_node = root;
  TrieNode *prev_node = NULL;
  TrieNode *next_node = NULL;
  uint8_t curr_sym = 0;
  uint8_t prev_sym = 0;
  uint16_t next_code = START_CODE;
  while (read_sym(infile, &curr_sym)) {
    next_node = trie_step(curr_node, curr_sym);
    if (next_node) {
      prev_node = curr_node;
      curr_node = next_node;
    } else {
      buffer_pair(outfile, curr_node->code, curr_sym, bitlength(next_code));
      curr_node->children[curr_sym] = trie_node_create(next_code);
      curr_node = root;
      next_code = next_code + 1;
    }
    if (next_code == MAX_CODE) {
      trie_reset(root);
      curr_node = root;
      next_code = START_CODE;
    }
    prev_sym = curr_sym;
  }
  if (curr_node != root) {
    buffer_pair(outfile, prev_node->code, prev_sym, bitlength(next_code));
    next_code = (next_code + 1) % MAX_CODE;
  }
  buffer_pair(outfile, STOP_CODE, 0, bitlength(next_code));
  flush_pairs(outfile);
  trie_reset(root);
  trie_node_delete(root);
  close(infile);
  close(outfile);
  if (stats) {
    printf("Compressed file size: %lu bytes\n", compressed_bits);
    printf("Uncompressed file size: %lu bytes\n", uncompressed_bits);
    printf("Compression ratio: %.2f%%\n",
        (100 * (1 - ((double)compressed_bits / uncompressed_bits))));
  }
  return 0;
}

//Does log change and adds 1 to calculate next code
uint8_t bitlength(uint16_t next_code) {
  double x = log(next_code) / log(2);
  uint8_t bt = (uint8_t)x + 1;
  return bt;
}
