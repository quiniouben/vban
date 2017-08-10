vban - Linux command-line VBAN tools
======================================================

&copy; 2015 Benoît Quiniou - quiniouben[at]yahoo(.)fr

vban project is an open-source implementation of VBAN protocol.
VBAN is a simple audio over UDP protocol proposed by VB-Audio, see [VBAN Audio webpage](http://vb-audio.pagesperso-orange.fr/Voicemeeter/vban.htm).
It is composed of several command-line tools allowing to stream audio coming from audio backend interfaces to VBAN stream (vban_emitter) or playout incoming VBAN stream to audio backend interfaces (vban_receptor)
Up to now, Alsa, PulseAudio and Jack audio backends have been implemented. A fifo (pipe) output is also existing, to allow chaining command-line tools, its status is experimental.

Compilation and installation
----------------------------

Depending on which audio backends you want to compile in, you need corresponding library and source headers.
Usual package names are:

* Alsa: libasound(X) and eventually libasound(X)-dev
* PulseAudio: libpulse(X) and eventually libpulse(X)-dev
* Jack: libjack(X) and eventually libjack(X)-dev

vban is distributed with autotools build scripts, therefore, to build, you need to invoke:

    $ ./autogen.sh              # probably only once for ever
    $ ./configure               # with or without options (--help to get the list of possible options)
    $ make                      # with or without options

To install, simply invoke:

    # make install

By default, vban tools will be compiled with all 3 audio backends. To disable them, configure options are:
    --disable-alsa
    --disable-pulse
    --disable-jack

Usage
-----

Invoking vban_receptor or vban_emitter without any parameter will give hints on how to use them :

Usage: vban_receptor [OPTIONS]...

	-i, --ipaddress=IP      : MANDATORY. ipaddress to get stream from
	-p, --port=PORT         : MANDATORY. port to listen to
	-s, --streamname=NAME   : MANDATORY. streamname to play
	-b, --backend=TYPE      : audio backend to use. possible values: alsa, pulseaudio, jack and pipe (EXPERIMENTAL). default is first in this list that is actually compiled
	-q, --quality=ID        : network quality indicator from 0 (low latency) to 4. default is 1
	-c, --channels=LIST     : channels from the stream to use. LIST is of form x,y,z,... default is to forward the stream as it is
	-o, --output=NAME       : Output device (server for jack backend) name , (as given by "aplay -L" output for alsa). using backend's default by default. not used for jack or pipe
	-d, --debug=LEVEL       : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)
	-h, --help              : display this message

TODO: missing vban_emitter doc

About --channels option, a bit more tips:
* channels indexes are from 1 to 256 (as specified by VBAN specifications for vban_receptor, and well, its probably enough for any soundcard or jack configuration)
* you can repeat channels
* with vban_receptor, if you use in-existent channels, you will get silence but no error (as the stream may change at anytime)
* with vban_emitter, if you use in-existent source channels, you will get error

Examples:

	vban_receptor -i IP -p PORT -s STREAMNAME -c1                   # keep only channel 1 and play out as mono
	vban_receptor -i IP -p PORT -s STREAMNAME -c1,1,1,1             # keep only channel 1 and play it out on 4 output channels (given that your output device is able to do it)
	vban_receptor -i IP -p PORT -s STREAMNAME -c2,41,125,7,1,45     # select some channels and play them out on 6 output channels (same comment)
    vban_emitter -i IP -p PORT -s STREAMNAME -c1,1,1,1               # use audio source channel 1 (opening it in mono therefore, and build up a 4 channels stream with copies of the same data in all channels)
