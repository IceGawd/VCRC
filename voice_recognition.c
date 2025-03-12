#include <time.h>
#include <portaudio.h>
#include <pocketsphinx.h>
#include <signal.h>

#include "numbers.h"

#ifdef __arm__ // If compiling for Raspberry Pi
	#include <wiringPi.h>
	#define FRONT_LEFT_PIN	9
	#define BACK_LEFT_PIN	8
	#define LEFT_EN_PIN		7
	#define ACTIVE_LIGHT    15
	#define FRONT_RIGHT_PIN	28
	#define BACK_RIGHT_PIN	27
	#define RIGHT_EN_PIN	29

	#define SETFREQ_SYSCALL_NUM 442

	static int maxFreq = 0;
	static int minFreq = 0;
	static int curFreq = 0;

	void init_userspace_governor() {
		char buf[32];

		FILE* fp;
		fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "w");
		fprintf(fp, "userspace");
		fclose(fp);


		fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", "r");
		fscanf(fp, "%d", &maxFreq);
		fclose(fp);

		fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq", "r");
		fscanf(fp, "%d", &minFreq);
		fclose(fp);

		fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", "r");
		fscanf(fp, "%d", &curFreq);
		fclose(fp);

		// Custom syscall test:
		// Instead of File IO, we use a custom system call to minimize FILE IO time.
		// Here, we test if it correctly works in the custom kernel.
		set_by_min_freq();

		fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
		fscanf(fp, "%d", &curFreq);
		fclose(fp);

		printf("MIN frequency change test: %d == %d\n", curFreq, minFreq);


		set_by_max_freq();

		fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
		fscanf(fp, "%d", &curFreq);
		fclose(fp);

		printf("MAX frequency change test: %d == %d\n", curFreq, maxFreq);
	}


	static void set_frequency(int frequency) {
		FILE* fp;
		fp = fopen(POLICY_PATH"scaling_setspeed", "w");
		fprintf(fp, "%d", frequency);
		fclose(fp);
	}

	void set_by_max_freq() {
	    assert(maxFreq > 0);
		set_frequency(maxFreq);
	}


	void set_by_min_freq() {
	    assert(minFreq > 0);
		set_frequency(minFreq);
	}
#endif

#define FORWARD  1
#define BACKWARD 2
#define LEFT     3
#define RIGHT    4
#define STOP     5
#define DEACTIVE 6

// Command synonym lists
const char* move_forward[] = {"move", "ahead", "forward"};
const char* move_backward[] = {"backward", "reverse", "back"};
const char* turn_left[] = {"left"};
const char* turn_right[] = {"right"};
const char* stop[] = {"stop", "halt", "pause"};
const char* activation[] = {"c", "r", "c"};
const char* deactivate[] = {"deactivate", "shutdown"};

const char* time_units[] = {"second", "seconds", "minute", "minutes"};

typedef struct ScheduledCommand {
	int command;
	time_t execute_at;
	struct ScheduledCommand* next;
} ScheduledCommand;

ScheduledCommand* command_queue = NULL;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

int matches_command(const char* recognized, const char* command_list[], int size) {	
	for (int i = 0; i < size; i++) {
		if (strcmp(recognized, command_list[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

const char** split_string(const char* str, int* count) {
	int words = 0;
	for (int i = 0; str[i] != '\0'; i++) {
		if (str[i] != ' ' && (i == 0 || str[i-1] == ' ')) {
			words++;
		}
	}

	char** result = (char**)malloc(words * sizeof(char*));
	*count = words;

	int j = 0;
	char* token = strtok(strdup(str), " ");
	while (token != NULL) {
		result[j++] = token;
		token = strtok(NULL, " ");
	}

	return result;
}

int search_activation(const char* recognized) {
	char* input_copy = strdup(recognized);

	int len = 0;

	char** separated = split_string(input_copy, &len);
	char* token;

	int depth = 0;

	for (int i = 0; i < len; i++) {
		token = separated[i];
		if (strcmp(activation[depth], token) == 0) {
			depth++;
			if (depth == 3) {
				return 1;
			}
		}
	}
	return 0;
}

void schedule_command(int command, int delay_seconds) {
	printf("Scheduling Command %d for %d seconds\n", command, delay_seconds);

	ScheduledCommand* new_command = (ScheduledCommand*) malloc(sizeof(ScheduledCommand));
	new_command->command = command;
	new_command->execute_at = time(NULL) + delay_seconds;
	new_command->next = NULL;

	pthread_mutex_lock(&queue_mutex);
	if (command_queue == NULL) {
		command_queue = new_command;
	} else {
		ScheduledCommand* current = command_queue;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = new_command;
	}
	pthread_mutex_unlock(&queue_mutex);
}

int find_command(const char* recognized) {
	char* input_copy = strdup(recognized);

	int command = 0;
	int len = 0;

	const char** separated = split_string(input_copy, &len);
	char* token;

	int fo = 0;
	int in = 0;
	int start = 0;
	int end = 0;

	for (int i = 0; i < len; i++) {
		token = separated[i];

		if (strcmp(token, "for") == 0) {
			in = 0;
			fo = 1;
			start = i + 1;
		}
		else if (strcmp(token, "in") == 0) {
			fo = 0;
			in = 1;
			start = i + 1;
		}
		else if (matches_command(token, move_forward, 3)) {
			end = i;
			command = FORWARD;
		}
		else if (matches_command(token, move_backward, 3)) {
			end = i;
			command = BACKWARD;
		}
		else if (matches_command(token, turn_left, 1)) {
			end = i;
			command = LEFT;
		}
		else if (matches_command(token, turn_right, 1)) {
			end = i;
			command = RIGHT;
		}
		else if (matches_command(token, stop, 3)) {
			end = i;
			command = STOP;
		}
		else if (matches_command(token, deactivate, 2)) {
			end = i;
			command = DEACTIVE;
		}
	}

	if (command != 0) {
		int duration = extract_duration(separated, start, end);
		if (duration != -1) {
			if (fo) {
				schedule_command(STOP, duration);			
			}
			else if (in) {
				schedule_command(command, duration);
				command = -1;			
			}
		}
	}

	free(input_copy);
	return command;
}


// /*
void execute_command(int command) {
	#ifdef __arm__ // RPi system
		if (command == FORWARD) {
			printf("Command: Move Forward\n");
			digitalWrite(FRONT_LEFT_PIN, HIGH);
			digitalWrite(FRONT_RIGHT_PIN, HIGH);
			digitalWrite(BACK_LEFT_PIN, LOW);
			digitalWrite(BACK_RIGHT_PIN, LOW);
		}
		else if (command == BACKWARD) {
			printf("Command: Move Backward\n");
			digitalWrite(FRONT_LEFT_PIN, LOW);
			digitalWrite(FRONT_RIGHT_PIN, LOW);
			digitalWrite(BACK_LEFT_PIN, HIGH);
			digitalWrite(BACK_RIGHT_PIN, HIGH);
		}
		else if (command == LEFT) {
			printf("Command: Turn Left\n");
			digitalWrite(FRONT_LEFT_PIN, HIGH);
			digitalWrite(FRONT_RIGHT_PIN, LOW);
			digitalWrite(BACK_LEFT_PIN, LOW);
			digitalWrite(BACK_RIGHT_PIN, HIGH);
		}
		else if (command == RIGHT) {
			printf("Command: Turn Right\n");
			digitalWrite(FRONT_LEFT_PIN, LOW);
			digitalWrite(FRONT_RIGHT_PIN, HIGH);
			digitalWrite(BACK_LEFT_PIN, HIGH);
			digitalWrite(BACK_RIGHT_PIN, LOW);
		}
		else if (command == STOP) {
			printf("Command: Stop\n");
			digitalWrite(FRONT_LEFT_PIN, LOW);
			digitalWrite(FRONT_RIGHT_PIN, LOW);
			digitalWrite(BACK_LEFT_PIN, LOW);
			digitalWrite(BACK_RIGHT_PIN, LOW);
		}
		else {
			printf("Unrecognized command: %d\n", command);
		}
	#else // Ubuntu system
		if (command == FORWARD) {
			printf("Command: Move Forward\n");
		}
		else if (command == BACKWARD) {
			printf("Command: Move Backward\n");
		}
		else if (command == LEFT) {
			printf("Command: Turn Left\n");
		}
		else if (command == RIGHT) {
			printf("Command: Turn Right\n");
		}
		else if (command == STOP) {
			printf("Command: Stop\n");
		}
		else {
			printf("Unrecognized command: %d\n", command);
		}
	#endif
}
// */

void recognize_and_execute_loop(PaStream* stream, ps_decoder_t* decoder, ps_endpointer_t* ep, short* frame, size_t frame_size) {
	PaError err;
	int activated = 1;

	#ifdef __arm__
		set_by_max_freq();
	#endif

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

				if (activated == 0) {
					fprintf(stderr, "DEACTIVE PARTIAL RESULT: %s\n", hyp);
					fflush(stderr);

					activated = search_activation(hyp);
					if (activated == 1) {
						ps_end_utt(decoder);
						ps_start_utt(decoder);

						#ifdef __arm__
							set_by_max_freq();
						#endif
					}
				}
				else {
					int command = find_command(hyp);
					fprintf(stderr, "ACTIVE PARTIAL RESULT: %s\n", hyp);
					fflush(stderr);
					// printf("%d\n", command);

					if (command == DEACTIVE) {
						activated = 0;
						ps_end_utt(decoder);
						ps_start_utt(decoder);

						#ifdef __arm__
							set_by_min_freq();
						#endif
					}
					else if (command != 0) {
						if (command != -1) {
							execute_command(command);
						}

						ps_end_utt(decoder);
						ps_start_utt(decoder);
					}
				}
			}
			if (!ps_endpointer_in_speech(ep)) {
				// fprintf(stderr, "Speech end at %.2f\n", ps_endpointer_speech_end(ep));
				// fflush(stderr);
				ps_end_utt(decoder);
				if ((hyp = ps_get_hyp(decoder, NULL)) != NULL) {
					// find_command(hyp);

					printf("%s\n", hyp);
					fflush(stdout);
				}
			}
		}
	}
}

int main() {
	// /*
	PaStream* stream;
	PaError err;
	ps_decoder_t* decoder;
	ps_config_t* config;
	ps_endpointer_t* ep;
	short* frame;
	size_t frame_size;

	#ifdef __arm__
		init_userspace_governor();
	#endif

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
	// */

	/*
	const char* words1[] = {"one", "hundred", "and", "twenty", "seconds"};
	const char* words2[] = {"thirty", "and", "eleven", "one", "seconds"};
	const char* words3[] = {"one", "thousand", "and", "twenty", "three", "seconds"};
	const char* words4[] = {"one", "hundred", "thousand", "twenty", "three", "seconds"};
	printf("%d\n", extract_duration(words1, 0, 5));
	printf("%d\n", extract_duration(words2, 0, 5));
	printf("%d\n", extract_duration(words3, 0, 6));
	printf("%d\n", extract_duration(words4, 0, 6));
	*/

	return 0;
}
