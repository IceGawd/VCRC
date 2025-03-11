UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M), x86_64)
    PLATFORM = ubuntu
else
    PLATFORM = rpi
endif

CC = gcc

SRCS = voice_recognition.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -O3 -Wall -Wextra 
INCLUDES = -I/usr/local/include
LIBS = -L/usr/local/lib -lpocketsphinx -lm -lportaudio

SRC = voice_recognition.c
OUT = voice_recognition

ifeq ($(PLATFORM), rpi)
    CFLAGS += -DPLATFORM_RPI
    LIBS += -lwiringPi
else
    CFLAGS += -DPLATFORM_UBUNTU
endif

TARGET = voice_recognition

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

clean:
	rm -f $(OUT)