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

bin_PROGRAMS = vban_receptor vban_emitter vban_sendtext
vban_receptor_SOURCES = receptor/main.c common/version.h \
						common/audio.h common/audio.c common/packet.h common/packet.c \
						common/backend/audio_backend.h common/backend/audio_backend.c \
						common/backend/pipe_backend.c common/backend/pipe_backend.h common/backend/file_backend.c common/backend/file_backend.h \
						common/socket.h common/socket.c common/stream.h common/stream.c \
						vban/vban.h common/logger.h common/logger.c

vban_emitter_SOURCES = emitter/main.c common/version.h \
						common/audio.h common/audio.c common/packet.h common/packet.c \
						common/backend/audio_backend.h common/backend/audio_backend.c \
						common/backend/pipe_backend.c common/backend/pipe_backend.h common/backend/file_backend.c common/backend/file_backend.h \
						common/socket.h common/socket.c common/stream.h common/stream.c \
						vban/vban.h common/logger.h common/logger.c

vban_sendtext_SOURCES = sendtext/main.c common/version.h \
						common/socket.h common/socket.c \
						vban/vban.h common/logger.h common/logger.c

if ALSA
vban_receptor_SOURCES += common/backend/alsa_backend.h common/backend/alsa_backend.c 
vban_emitter_SOURCES += common/backend/alsa_backend.h common/backend/alsa_backend.c 
endif

if PULSEAUDIO
vban_receptor_SOURCES += common/backend/pulseaudio_backend.h common/backend/pulseaudio_backend.c
vban_emitter_SOURCES += common/backend/pulseaudio_backend.h common/backend/pulseaudio_backend.c
endif

if JACK
vban_receptor_SOURCES += common/backend/jack_backend.h common/backend/jack_backend.c
vban_emitter_SOURCES += common/backend/jack_backend.h common/backend/jack_backend.c
endif
