# Let's define some things here
AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS = -std=c99 -pedantic -Wall -Wno-multichar -O2 -I.
AM_LDFLAGS = 

if ALSA
AM_LDFLAGS += -lasound 
AM_CFLAGS += -DALSA
endif

if PULSEAUDIO
AM_LDFLAGS += -lpulse -lpulse-simple
AM_CFLAGS += -DPULSEAUDIO
endif

if JACK
AM_LDFLAGS += -ljack
AM_CFLAGS += -DJACK
endif

bin_PROGRAMS = vban_receptor vban_source
vban_receptor_SOURCES = receptor/main.c \
						audio/audio.h audio/audio.c \
						audio/audio_backend.h audio/audio_backend.c audio/pipe_backend.c audio/pipe_backend.h file_backend.c file_backend.h \
						net/socket.h net/socket.c \
						vban/vban.h util/logger.h util/logger.c

vban_source_SOURCES = source/main.c \
						audio/audio.h audio/audio.c \
						audio/audio_backend.h audio/audio_backend.c audio/pipe_backend.c audio/pipe_backend.h file_backend.c file_backend.h \
						net/socket.h net/socket.c \
						vban/vban.h util/logger.h util/logger.c

if ALSA
vban_receptor_SOURCES += audio/alsa_backend.h audio/alsa_backend.c 
vban_source_SOURCES += audio/alsa_backend.h audio/alsa_backend.c 
endif

if PULSEAUDIO
vban_receptor_SOURCES += audio/pulseaudio_backend.h audio/pulseaudio_backend.c
vban_source_SOURCES += audio/pulseaudio_backend.h audio/pulseaudio_backend.c
endif

if JACK
vban_receptor_SOURCES += audio/jack_backend.h audio/jack_backend.c
vban_source_SOURCES += audio/jack_backend.h audio/jack_backend.c
endif
