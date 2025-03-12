#ifndef NUMBERS_H
#define NUMBERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>

typedef struct {
	const char* word;
	int value;
} NumberWord;

extern NumberWord units[];
extern NumberWord teens[];
extern NumberWord tens[];
extern NumberWord magnitudes[];

int word_to_number(const char* word);
int magnitude_value(const char* word);
int convert_number_words_to_int(const char* input);

#endif