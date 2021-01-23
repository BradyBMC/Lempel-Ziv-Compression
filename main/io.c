#include "io.h"
#include "code.h"
#include <unistd.h>

//Buffer for pair and symbols
//
static uint8_t pair_buff[4096];
static uint16_t pair_buff_index = 0;

static uint8_t sym_buff[4096];
static uint16_t sym_buff_index = 0;

uint64_t uncompressed_bits = 0;
uint64_t compressed_bits = 0;

//
// Wrapper for the read() syscall.
// Loops to read the specified number of bytes, or until input is exhausted.
// Returns the number of bytes read.
//
// infile:  File descriptor of the input file to read from.
// buf:     Buffer to store read bytes into.
// to_read: Number of bytes to read.
// returns: Number of bytes read.
//
int read_bytes(int infile, uint8_t *buf, int to_read) {
  int bytes_read = 2;
  int cnt = to_read;
  int total = 0;
  while ((total != to_read) && (bytes_read != 0)) {
    //Reads in byte from infile
    bytes_read = read(infile, buf + total, cnt);
    total += bytes_read;
    cnt -= bytes_read;
  }
  uncompressed_bits += total;
  return total;
}

//
// Wrapper for the write() syscall.
// Loops to write the specified number of bytes, or until nothing is written.
// Returns the number of bytes written.
//
// outfile:   File descriptor of the output file to write to.
// buf:       Buffer that stores the bytes to write out.
// to_write:  Number of bytes to write.
// returns:   Number of bytes written.
//
int write_bytes(int outfile, uint8_t *buf, int to_write) {
  int bytes_write = 2;
  int cnt = to_write;
  int total = 0;
  while ((total != to_write) && (bytes_write != 0)) {
    bytes_write = write(outfile, buf + total, cnt);
    total += bytes_write;
    cnt -= bytes_write;
  }
  compressed_bits += total;
  return total;
}

//
// Reads in a FileHeader from the input file.
//
// infile:  File descriptor of input file to read header from.
// header:  Pointer to memory where the bytes of the read header should go.
// returns: Void.
//
void read_header(int infile, FileHeader *header) {
  read_bytes(infile, (uint8_t *)header, sizeof(FileHeader));
  return;
}

//
// Writes a FileHeader to the output file.
//
// outfile: File descriptor of output file to write header to.
// header:  Pointer to the header to write out.
// returns: Void.
//
void write_header(int outfile, FileHeader *header) {
  write_bytes(outfile, (uint8_t *)header, sizeof(FileHeader));
  return;
}

//
// "Reads" a symbol from the input file.
// The "read" symbol is placed into the pointer to sym (pass by reference).
// In reality, a block of symbols is read into a buffer.
// An index keeps track of the currently read symbol in the buffer.
// Once all symbols are processed, another block is read.
// If less than a block is read, the end of the buffer is updated.
// Returns true if there are symbols to be read, false otherwise.
//
// infile:  File descriptor of input file to read symbols from.
// sym:     Pointer to memory which stores the read symbol.
// returns: True if there are symbols to be read, false otherwise.
//
bool read_sym(int infile, uint8_t *byte) {
  static int num_of_bytes_read = 0;
  //Checks if empty and fills buffer
  if (sym_buff_index == 0) {
    num_of_bytes_read = read_bytes(infile, sym_buff, 4096);
  }
  *byte = sym_buff[sym_buff_index];
  sym_buff_index++;
  //If at end reset index
  if (sym_buff_index == 4096) {
    sym_buff_index = 0;
  }
  //If index goes beyond read, end of file
  if (sym_buff_index > num_of_bytes_read) {
    return false;
  }
  return true;
}

//
// Buffers a pair. A pair is comprised of a symbol and an index.
// The bits of the symbol are buffered first, starting from the LSB.
// The bits of the index are buffered next, also starting from the LSB.
// bit_len bits of the index are buffered to provide a minimal representation.
// The buffer is written out whenever it is filled.
//
// outfile: File descriptor of the output file to write to.
// sym:     Symbol of the pair to buffer.
// index:   Index of the pair to buffer.
// bit_len: Number of bits of the index to buffer.
// returns: Void.
//
void buffer_pair(int outfile, uint16_t code, uint8_t sym, uint8_t bitlen) {
  for (uint16_t i = 0; i < bitlen; i++) {
    //Shifts 1 to index in code then performs the & to checks if bit is 1
    if (code & (1 << i)) {
      pair_buff[pair_buff_index / 8] |= (1 << (pair_buff_index % 8));
    } else {
      pair_buff[pair_buff_index / 8] &= ~(1 << (pair_buff_index % 8));
    }
    //Increments to check next pair in the buffer
    pair_buff_index++;
    //Takes a new chunk if its completed and resets index
    if (pair_buff_index / 8 == 4096) {
      write_bytes(outfile, pair_buff, 4096);
      pair_buff_index = 0;
    }
  }
  for (uint8_t i = 0; i < 8; i++) {
    if (sym & (1 << i)) {
      pair_buff[pair_buff_index / 8] |= (1 << (pair_buff_index % 8));
    } else {
      pair_buff[pair_buff_index / 8] &= ~(1 << (pair_buff_index % 8));
    }
    pair_buff_index++;
    if (pair_buff_index / 8 == 4096) {
      write_bytes(outfile, pair_buff, 4096);
      pair_buff_index = 0;
    }
  }
  return;
}

//
// Writes out any remaining pairs of symbols and indexes to the output file.
//
// outfile: File descriptor of the output file to write to.
// returns: Void.
//
void flush_pairs(int outfile) {
  int to_write = 0;
  //Calculates bytes to pass to write_bytes
  if (pair_buff_index % 8 == 0) {
    to_write = pair_buff_index / 8;
  } else {
    to_write = pair_buff_index / 8 + 1;
  }
  if (pair_buff_index != 0) {
    write_bytes(outfile, pair_buff, to_write);
    pair_buff_index = 0;
  }
  return;
}

//
// "Reads" a pair (symbol and index) from the input file.
// The "read" symbol is placed in the pointer to sym (pass by reference).
// The "read" index is placed in the pointer to index (pass by reference).
// In reality, a block of pairs is read into a buffer.
// An index keeps track of the current bit in the buffer.
// Once all bits have been processed, another block is read.
// The first 8 bits of the pair constitute the symbol, starting from the LSB.
// The next bit_len bits constitutes the index, starting from the the LSB.
// Returns true if there are pairs left to read in the buffer, else false.
// There are pairs left to read if the read index is not STOP_INDEX.
//
// infile:  File descriptor of the input file to read from.
// sym:     Pointer to memory which stores the read symbol.
// index:   Pointer to memory which stores the read index.
// bit_len: Length in bits of the index to read.
// returns: True if there are pairs left to read, false otherwise.
//
bool read_pair(int infile, uint16_t *code, uint8_t *sym, uint8_t bitlen) {
  //Clears code and sym
  *code = 0;
  *sym = 0;
  //Uses the same logic as buffer_pair except adds it to code and sym
  for (uint16_t i = 0; i < bitlen; i++) {
    if (pair_buff_index == 0) {
      read_bytes(infile, pair_buff, 4096);
    }
    if ((pair_buff[pair_buff_index / 8] >> (pair_buff_index % 8)) & 1) {
      *code |= (1 << i);
    } else
      *code &= ~(1 << i);
    pair_buff_index++;
    if (pair_buff_index / 8 == 4096) {
      pair_buff_index = 0;
    }
  }
  for (uint8_t i = 0; i < 8; i++) {
    if (pair_buff_index == 0) {
      read_bytes(infile, pair_buff, 4096);
    }
    if ((pair_buff[pair_buff_index / 8] >> (pair_buff_index % 8)) & 1) {
      *sym |= (1 << i);
    } else {
      *sym &= ~(1 << i);
    }
    pair_buff_index++;
    if (pair_buff_index / 8 == 4096) {
      pair_buff_index = 0;
    }
  }
  return *code != STOP_CODE;
}

//
// Buffers a Word, or more specifically, the symbols of a Word.
// Each symbol of the Word is placed into a buffer.
// The buffer is written out when it is filled.
//
// outfile: File descriptor of the output file to write to.
// w:       Word to buffer.
// returns: Void.
//
void buffer_word(int outfile, Word *w) {
  //Adds all syms from word to buffer
  for (uint32_t i = 0; i < w->len; i++) {
    sym_buff[sym_buff_index] = w->syms[i];
    sym_buff_index++;
    if (sym_buff_index == 4096) {
      write_bytes(outfile, sym_buff, 4096);
      sym_buff_index = 0;
    }
  }
  return;
}

//
// Writes out any remaining symbols in the buffer.
//
// outfile: File descriptor of the output file to write to.
// returns: Void.
//
void flush_words(int outfile) {
  if (sym_buff_index != 0) {
    write_bytes(outfile, sym_buff, sym_buff_index);
    sym_buff_index = 0;
  }
  return;
}
