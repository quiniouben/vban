# Let's define some things here
AM_CFLAGS = -std=c99 -Wpedantic -Wall -Wno-multichar
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

bin_PROGRAMS = vban_receptor
vban_receptor_SOURCES = main.c audio.h audio.c socket.h socket.c vban.h logger.h logger.c audio_backend.h audio_backend.c pipe_backend.c pipe_backend.h

if ALSA
vban_receptor_SOURCES += alsa_backend.h alsa_backend.c 
endif

if PULSEAUDIO
vban_receptor_SOURCES +=pulseaudio_backend.h pulseaudio_backend.c
endif

if JACK
vban_receptor_SOURCES +=jack_backend.h jack_backend.c
endif
