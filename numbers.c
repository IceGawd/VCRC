#include "numbers.h"

NumberWord units[] = {
	{"zero", 0}, {"one", 1}, {"two", 2}, {"three", 3}, {"four", 4},
	{"five", 5}, {"six", 6}, {"seven", 7}, {"eight", 8}, {"nine", 9},
	{NULL, 0}
};
NumberWord teens[] = {
	{"ten", 10}, {"eleven", 11}, {"twelve", 12}, {"thirteen", 13}, {"fourteen", 14},
	{"fifteen", 15}, {"sixteen", 16}, {"seventeen", 17}, {"eighteen", 18}, {"nineteen", 19},
	{NULL, 0}
};
NumberWord tens[] = {
	{"twenty", 20}, {"thirty", 30}, {"forty", 40}, {"fifty", 50},
	{"sixty", 60}, {"seventy", 70}, {"eighty", 80}, {"ninety", 90},
	{NULL, 0}
};
NumberWord magnitudes[] = {
	{"hundred", 100}, {"thousand", 1000}, {"million", 1000000},
	{NULL, 0}
};

int word_to_number(const char* word) {
	for (NumberWord* nw = units; nw->word != NULL; ++nw) {
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	for (NumberWord* nw = teens; nw->word != NULL; ++nw) {
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	for (NumberWord* nw = tens; nw->word != NULL; ++nw) {
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	return -1;
}

int magnitude_value(const char* word) {
	for (NumberWord* nw = magnitudes; nw->word != NULL; ++nw) {
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	return -1;
}

int convert_number_words_to_int(const char* input) {
	char* token;
	char* input_copy = strdup(input);
	int result = 0, current = 0;
	char* rest = input_copy;

	while ((token = strtok_r(rest, " -", &rest))) {
		for (char* p = token;* p; ++p)* p = tolower(*p); // Convert to lowercase

		int num = word_to_number(token);
		if (num != -1) {
			current += num;
		}
		else {
			int mag = magnitude_value(token);
			if (mag != -1) {
				if (mag == 100) {
					current *= mag;
				} else {
					current *= mag;
					result += current;
					current = 0;
				}
			} else {
				fprintf(stderr, "Unknown number word: %s\n", token);
				free(input_copy);
				return -1;
			}
		}
	}
	result += current;
	free(input_copy);
	return result;
}