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

int word_to_number(const char* word, int* level) {
	for (NumberWord* nw = units; nw->word != NULL; ++nw) {
		*level = 0;
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	for (NumberWord* nw = teens; nw->word != NULL; ++nw) {
		*level = 1;
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	for (NumberWord* nw = tens; nw->word != NULL; ++nw) {
		*level = 1;
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	*level = -1;
	return -1;
}

int magnitude_value(const char* word) {
	for (NumberWord* nw = magnitudes; nw->word != NULL; ++nw) {
		if (strcmp(nw->word, word) == 0) return nw->value;
	}
	return -1;
}

int extract_duration(const char** separated, int start, int finish) {
	int duration = 0;
	int temp = 0;
	int unit = 0;

	int level = 0;
	int prevLevel = 3;

	for (int i = start; i < finish; i++) {
		const char* token = separated[i];

		if (strcmp(token, "and")) {
			int num = word_to_number(token, &level);
			int mag = magnitude_value(token);
			if (num != -1) {
				temp += num;
				if (prevLevel <= level) {
					return -1;
				}
			}
			else if (mag != -1) {
				temp *= mag;
				level = 2;
			}
			else if ((strcmp(token, "second") == 0 || strcmp(token, "seconds") == 0) && unit == 0) {
				// Duration is already in seconds
				unit = 1;
				duration += temp;
				return (duration > 0) ? duration : -1;
			}
			else if ((strcmp(token, "minute") == 0 || strcmp(token, "minutes") == 0) && unit == 0) {
				unit = 1;
				duration += temp;
				duration *= 60;
				return (duration > 0) ? duration : -1;
			}
			else {
				return -1;
			}
		}

		prevLevel = level;
	}

	duration += temp;

	return (unit) ? duration : -1;
}
