#include "word.h"
#include "code.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//
// Constructor for a word.
//
// syms:    Array of symbols a Word represents.
// len:     Length of the array of symbols.
// returns: Pointer to a Word that has been allocated memory.
//
Word *word_create(uint8_t *syms, uint32_t len) {
  Word *word = (Word *)malloc(sizeof(Word));
  if (!word) {
    return 0;
  }
  //Allocate memory for all syms
  word->syms = (uint8_t *)calloc(len, sizeof(uint8_t));
  if (!word->syms) {
    free(word);
    return 0;
  }
  //Copies over syms to new word
  memcpy(word->syms, syms, len);
  word->len = len;
  return word;
}
//
// Constructs a new Word from the specified Word appended with a symbol.
// The Word specified to append to may be empty.
// If the above is the case, the new Word should contain only the symbol.
//
// w:       Word to append to.
// sym:     Symbol to append.
// returns: New Word which represents the result of appending.
Word *word_append_sym(Word *w, uint8_t sym) {
  if (w->len) {
    Word *new_word = (Word *)malloc(sizeof(Word));
    if (!new_word) {
      return 0;
    }
    new_word->syms = (uint8_t *)calloc(w->len + 1, sizeof(uint8_t));
    if (!new_word->syms) {
      free(new_word);
      return 0;
    }
    //Sets all syms in previos word in the new word
    for (uint32_t i = 0; i < w->len; i++) {
      new_word->syms[i] = w->syms[i];
    }
    //Adds the new sym to the end
    new_word->syms[w->len] = sym;
    new_word->len = w->len + 1;
    return new_word;
  } else {
    return word_create(&sym, 1);
  }
}

//Frees word and all memory allocated
//
//w: word being deleted
void word_delete(Word *w) {
  free(w->syms);
  free(w);
  return;
}

WordTable *wt_create(void) {
  WordTable *wt = (WordTable *)calloc(MAX_CODE, sizeof(Word *));
  if (!wt) {
    return 0;
  }
  wt[EMPTY_CODE] = (Word *)calloc(1, sizeof(Word));
  return wt;
}

//Clears everything in word table
//
//wt: wordtable being cleared
void wt_reset(WordTable *wt) {
  for (uint16_t i = 2; i < MAX_CODE; i++) {
    if (wt[i]) {
      word_delete(wt[i]);
    }
  }
}

//Deletes all of word table and words
//
//wt: wordtable being deleted
void wt_delete(WordTable *wt) {
  wt_reset(wt);
  free(wt[EMPTY_CODE]);
  free(wt);
  return;
}
