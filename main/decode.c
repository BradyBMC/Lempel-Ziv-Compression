#include "code.h"
#include "io.h"
#include "word.h"
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

uint8_t bitlen(uint16_t next_code);
extern uint64_t compressed_bits;
extern uint64_t uncompressed_bits;

int main(int argc, char **argv) {
  int c;
  int infile = 0;
  int outfile = 1;
  bool stats = false;
  bool infile_bol = false;
  bool outfile_bol = false;
  char *infile_name;
  char *outfile_name;
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
  //Avoids make infer error by opening outside of while
  if (infile_bol) {
    infile = open(infile_name, O_RDONLY);
    if (infile == -1) {
      puts("Error no file entered");
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
  read_header(infile, &hd);
  if (hd.magic != 0x8badbeef) {
    printf("Error Different Header\n");
    return 0;
  }
  fchmod(outfile, hd.protection);

  WordTable *table = wt_create();
  uint8_t curr_sym = 0;
  uint16_t curr_code = 0;
  uint16_t next_code = START_CODE;
  while (read_pair(infile, &curr_code, &curr_sym, bitlen(next_code))) {
    table[next_code] = word_append_sym(table[curr_code], curr_sym);
    buffer_word(outfile, table[next_code]);
    next_code = next_code + 1;
    if (next_code == MAX_CODE) {
      wt_reset(table);
      next_code = START_CODE;
    }
  }
  flush_words(outfile);
  wt_delete(table);
  close(infile);
  close(outfile);
  if (stats) {
    printf("Compressed file size: %lu bytes\n", uncompressed_bits);
    printf("Uncompressed file size: %lu bytes\n", compressed_bits);
    printf("Compression ratio: %.2f%%\n",
        (100 * (1 - ((double)uncompressed_bits / compressed_bits))));
  }
  return 0;
}

//Calculates next code by log and add 1
uint8_t bitlen(uint16_t next_code) {
  double x = log(next_code) / log(2);
  uint8_t bt = (uint8_t)x + 1;
  return bt;
}
