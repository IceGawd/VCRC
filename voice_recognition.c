#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <portaudio.h>
#include <pocketsphinx.h>
#include <signal.h>
#include <ctype.h>

#ifdef __arm__ // If compiling for Raspberry Pi
	#include <wiringPi.h>
	#define FRONT_LEFT_PIN  17
	#define FRONT_RIGHT_PIN 27
	#define BACK_LEFT_PIN   22
	#define BACK_RIGHT_PIN  23
#endif

// Command synonym lists
const char* move_forward[] = {"move", "go", "ahead", "forward"};
const char* move_backward[] = {"backward", "reverse", "back"};
const char* turn_left[] = {"left", "turn left"};
const char* turn_right[] = {"right", "turn right"};
const char* stop[] = {"stop", "halt", "pause"};
const char* activation[] = {"v", "c", "r"};

typedef struct {
    const char* word;
    int value;
} NumberWord;
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
        } else {
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

int matches_command(const char* recognized, const char* command_list[], int size) {
	for (int i = 0; i < size; i++) {
		if (strcmp(recognized, command_list[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

void execute_command(const char* command) {
	#ifdef __arm__ // RPi system
	    if (matches_command(command, move_forward, sizeof(move_forward)/sizeof(move_forward[0]))) {
	        printf("Command: Move Forward\n");
	        digitalWrite(FRONT_LEFT_PIN, HIGH);
	        digitalWrite(FRONT_RIGHT_PIN, HIGH);
	        digitalWrite(BACK_LEFT_PIN, LOW);
	        digitalWrite(BACK_RIGHT_PIN, LOW);
	    } else if (matches_command(command, move_backward, sizeof(move_backward)/sizeof(move_backward[0]))) {
	        printf("Command: Move Backward\n");
	        digitalWrite(FRONT_LEFT_PIN, LOW);
	        digitalWrite(FRONT_RIGHT_PIN, LOW);
	        digitalWrite(BACK_LEFT_PIN, HIGH);
	        digitalWrite(BACK_RIGHT_PIN, HIGH);
	    } else if (matches_command(command, turn_left, sizeof(turn_left)/sizeof(turn_left[0]))) {
	        printf("Command: Turn Left\n");
	        digitalWrite(FRONT_LEFT_PIN, HIGH);
	        digitalWrite(FRONT_RIGHT_PIN, LOW);
	        digitalWrite(BACK_LEFT_PIN, LOW);
	        digitalWrite(BACK_RIGHT_PIN, HIGH);
	    } else if (matches_command(command, turn_right, sizeof(turn_right)/sizeof(turn_right[0]))) {
	        printf("Command: Turn Right\n");
	        digitalWrite(FRONT_LEFT_PIN, LOW);
	        digitalWrite(FRONT_RIGHT_PIN, HIGH);
	        digitalWrite(BACK_LEFT_PIN, HIGH);
	        digitalWrite(BACK_RIGHT_PIN, LOW);
	    } else if (matches_command(command, stop, sizeof(stop)/sizeof(stop[0]))) {
	        printf("Command: Stop\n");
	        digitalWrite(FRONT_LEFT_PIN, LOW);
	        digitalWrite(FRONT_RIGHT_PIN, LOW);
	        digitalWrite(BACK_LEFT_PIN, LOW);
	        digitalWrite(BACK_RIGHT_PIN, LOW);
	    } else {
	        printf("Unrecognized command: %s\n", command);
	    }
	#else // Ubuntu system
	    if (matches_command(command, move_forward, sizeof(move_forward)/sizeof(move_forward[0]))) {
	        printf("Command: Move Forward\n");
	    } else if (matches_command(command, move_backward, sizeof(move_backward)/sizeof(move_backward[0]))) {
	        printf("Command: Move Backward\n");
	    } else if (matches_command(command, turn_left, sizeof(turn_left)/sizeof(turn_left[0]))) {
	        printf("Command: Turn Left\n");
	    } else if (matches_command(command, turn_right, sizeof(turn_right)/sizeof(turn_right[0]))) {
	        printf("Command: Turn Right\n");
	    } else if (matches_command(command, stop, sizeof(stop)/sizeof(stop[0]))) {
	        printf("Command: Stop\n");
	    } else {
	        printf("Unrecognized command: %s\n", command);
	    }
	#endif
}

void recognize_and_execute_loop(PaStream* stream, ps_decoder_t* decoder, ps_endpointer_t* ep, short* frame, size_t frame_size) {
	PaError err;

	while (1) {
		const int16* speech;
		int prev_in_speech = ps_endpointer_in_speech(ep);
		if ((err = Pa_ReadStream(stream, frame, frame_size)) != paNoError) {
			E_ERROR("Error in PortAudio read: %s\n", Pa_GetErrorText(err));
			break;
		}
		speech = ps_endpointer_process(ep, frame);
		if (speech != NULL) {
			const char* hyp;
			if (!prev_in_speech) {
				// fprintf(stderr, "Speech start at %.2f\n", ps_endpointer_speech_start(ep));
				// fflush(stderr);
				ps_start_utt(decoder);
			}
			if (ps_process_raw(decoder, speech, frame_size, FALSE, FALSE) < 0) {
				E_FATAL("ps_process_raw() failed\n");
			}
			if ((hyp = ps_get_hyp(decoder, NULL)) != NULL) {
				// fprintf(stderr, "PARTIAL RESULT: %s\n", hyp);
				// fflush(stderr);
			}
			if (!ps_endpointer_in_speech(ep)) {
				fprintf(stderr, "Speech end at %.2f\n", ps_endpointer_speech_end(ep));
				fflush(stderr);
				ps_end_utt(decoder);
				if ((hyp = ps_get_hyp(decoder, NULL)) != NULL) {
					execute_command(hyp);

					printf("%s\n", hyp);
					fflush(stdout);
				}
			}
		}
	}
}

int main() {
	PaStream* stream;
	PaError err;
	ps_decoder_t* decoder;
	ps_config_t* config;
	ps_endpointer_t* ep;
	short* frame;
	size_t frame_size;

	config = ps_config_init(NULL);
	ps_default_search_args(config);
	ps_config_set_str(config, "dict", "./commands.dict");

	if ((err = Pa_Initialize()) != paNoError) {
		E_FATAL("Failed to initialize PortAudio: %s\n", Pa_GetErrorText(err));
	}
	if ((decoder = ps_init(config)) == NULL){
		E_FATAL("PocketSphinx decoder init failed\n");
	}
	if ((ep = ps_endpointer_init(0, 0.0, 0, 0, 0)) == NULL){
		E_FATAL("PocketSphinx endpointer init failed\n");
	}

	frame_size = ps_endpointer_frame_size(ep);
	
	if ((frame = malloc(frame_size*  sizeof(frame[0]))) == NULL){
		E_FATAL_SYSTEM("Failed to allocate frame");
	}
	if ((err = Pa_OpenDefaultStream(&stream, 1, 0, paInt16, ps_config_int(config, "samprate"), frame_size, NULL, NULL)) != paNoError){
		E_FATAL("Failed to open PortAudio stream: %s\n", Pa_GetErrorText(err));
	}
	if ((err = Pa_StartStream(stream)) != paNoError){
		E_FATAL("Failed to start PortAudio stream: %s\n", Pa_GetErrorText(err));
	}

	recognize_and_execute_loop(stream, decoder, ep, frame, frame_size);

	if ((err = Pa_StopStream(stream)) != paNoError)
		E_FATAL("Failed to stop PortAudio stream: %s\n",
				Pa_GetErrorText(err));
	if ((err = Pa_Terminate()) != paNoError)
		E_FATAL("Failed to terminate PortAudio: %s\n",
				Pa_GetErrorText(err));
	free(frame);
	ps_endpointer_free(ep);
	ps_free(decoder);
	ps_config_free(config);
		
	return 0;
}
