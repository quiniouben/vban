vban - Linux command-line VBAN tools
======================================================

&copy; 2015 Benoît Quiniou - quiniouben[at]yahoo(.)fr

vban project is an open-source implementation of VBAN protocol.
VBAN is a simple audio over UDP protocol proposed by VB-Audio, see [VBAN Audio webpage](https://www.vb-audio.com/Voicemeeter/vban.htm)
It is composed of several command-line tools allowing to stream audio coming from audio backend interfaces to VBAN stream (vban_emitter) or playout incoming VBAN stream to audio backend interfaces (vban_receptor), or send text over the vban protocol (vban_sendtext).
Up to now, for audio tools, Alsa, PulseAudio and Jack audio backends have been implemented. A fifo (pipe) output is also existing, to allow chaining command-line tools, and a file output too (writing raw pcm data).

Compilation and installation
----------------------------

Depending on which audio backends you want to compile in, you need corresponding library and source headers.
Usual package names are:

* Alsa: libasound(X) and eventually libasound(X)-dev
* PulseAudio: libpulse(X) and eventually libpulse(X)-dev
* Jack: libjack(X) and eventually libjack(X)-dev

vban is distributed with autotools build scripts, therefore, to build, you need to install autoconf and automake, and to invoke:

	$ ./autogen.sh              # probably only once for ever
	$ ./configure               # with or without options (--help to get the list of possible options)
	$ make                      # with or without options

To install, simply invoke:

    # make install

By default, vban tools will be compiled with all 3 audio backends. To disable them, configure options are:
    --disable-alsa
    --disable-pulseaudio
    --disable-jack

Usage
-----

Invoking vban_receptor or vban_emitter without any parameter will give hints on how to use them :

	Usage: vban_receptor [OPTIONS]...

	-i, --ipaddress=IP      : MANDATORY. ipaddress to get stream from
	-p, --port=PORT         : MANDATORY. port to listen to
	-s, --streamname=NAME   : MANDATORY. streamname to play
	-b, --backend=TYPE      : audio backend to use. Available audio backends are: alsa pulseaudio jack pipe file . default is alsa.
	-q, --quality=ID        : network quality indicator from 0 (low latency) to 4. This also have interaction with jack buffer size. default is 1
	-c, --channels=LIST     : channels from the stream to use. LIST is of form x,y,z,... default is to forward the stream as it is
	-o, --output=NAME       : DEPRECATED. please use -d
	-d, --device=NAME       : Audio device name. This is file name for file backend, server name for jack backend, device for alsa, stream_name for pulseaudio.
	-l, --loglevel=LEVEL    : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)
	-h, --help              : display this message

	Usage: vban_emitter [OPTIONS]...

	-i, --ipaddress=IP      : MANDATORY. ipaddress to send stream to
	-p, --port=PORT         : MANDATORY. port to use
	-s, --streamname=NAME   : MANDATORY. streamname to use
	-b, --backend=TYPE      : TEMPORARY DISABLED. audio backend to use. Only alsa backend is working at this time
	-d, --device=NAME       : Audio device name. This is file name for file backend, server name for jack backend, device for alsa, stream_name for pulseaudio.
	-r, --rate=VALUE        : Audio device sample rate. default 44100
	-n, --nbchannels=VALUE  : Audio device number of channels. default 2
	-f, --format=VALUE      : Audio device sample format (see below). default is 16I (16bits integer)
	-c, --channels=LIST     : channels from the stream to use. LIST is of form x,y,z,... default is to forward the stream as it is
	-l, --loglevel=LEVEL	: Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)
	-h, --help	          : display this message

	Recognized bit format are 8I, 16I, 24I, 32I, 32F, 64F, 12I, 10I

	Usage: vban_emitter [OPTIONS] MESSAGE

	-i, --ipaddress=IP	  : MANDATORY. ipaddress to send stream to
	-p, --port=PORT         : MANDATORY. port to use
	-s, --streamname=NAME   : MANDATORY. streamname to use
	-b, --bps=VALUE         : Data bitrate indicator. default 0 (no special bitrate)
	-n, --ident=VALUE       : Subchannel identification. default 0
	-f, --format=VALUE      : Text format used. can be: 0 (ASCII), 1 (UTF8), 2 (WCHAR), 240 (USER). default 1
	-l, --loglevel=LEVEL    : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)
	-h, --help              : display this message



About --channels option, a bit more tips:
* channels indexes are from 1 to 256 (as specified by VBAN specifications, and well, its probably enough for any soundcard or jack configuration)
* you can repeat channels
* if you use in-existent channels, you will get silence but no error

Examples:

	vban_receptor -i IP -p PORT -s STREAMNAME -c1                   # keep only channel 1 and play out as mono
	vban_receptor -i IP -p PORT -s STREAMNAME -c1,1,1,1             # keep only channel 1 and play it out on 4 output channels (given that your output device is able to do it)
	vban_receptor -i IP -p PORT -s STREAMNAME -c2,41,125,7,1,45     # select some channels and play them out on 6 output channels (same comment)
	vban_emitter -i IP -p PORT -s STREAMNAME -c1,1,1,1               # use audio source channel 1 (opening it in mono therefore, and build up a 4 channels stream with copies of the same data in all channels)

LATENCY
-------

vban_receptor does its best to keep latency reasonable, according to the -q (--quality) parameter.
A buffer size is computed according to the quality parameter, following the recommandation of VBAN Protocol specification document.
Then:
* data is read from / written to network in chunks of buffer size
* for alsa, buffer size is used to require an adequate latency
* for pulseaudio, it is directly used to set the stream buffer size
* for jack, it is used to set an internal buffer size to the double

GUI
---

This project is only componed of command line tools. If you are looking for a gui, you can take a look at: [VBAN-manager project on GitHub](https://github.com/VBAN-manager/VBAN-manager)
