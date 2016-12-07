vban_receptor - Linux command-line VBAN to alsa player
======================================================

&copy; 2015 Benoît Quiniou - quiniouben[at]yahoo(.)fr

vban_receptor is an open-source implementation of VBAN protocol.
It currently takes the form of a Linux command-line player to play a VBAN audio stream to alsa or pulseaudio local audio device
VBAN is a simple audio over UDP protocol proposed by VB-Audio, see [VBAN Audio webpage](http://vb-audio.pagesperso-orange.fr/Voicemeeter/vban.htm).

Compilation and installation
----------------------------

vban_receptor uses either ALSA or PulseAudio, therefore you need to have ALSA  and /or PulseAudio library and headers available on your platform.
In standard Linux distributions, ALSA library package is usually named libasound(X) and header files come in same package or in a separate one named libasound(X)-dev.
On its side, PulseAUdio library package can be named libpulse and header files come in same package or in libpulse-dev.
If you use configure script with default option, both ALSA and PulseAudio libraries will be needed.

vban_receptor is distributed with autotools build scripts, therefore, to build, you need to invoke:

    $ ./autogen.sh              # probably only once for ever
    $ ./configure               # with or without options
    $ make                      # with or without options

To install, simply invoke:

    # make install

If you want to disable Pulseaudio backend, at configure script, invoke:

	$ ./configure --disable-pulseaudio


Usage
-----

Invoking vban_receptor without any parameter will give hints on how to use it :

	-i, --ipaddress=IP      : MANDATORY. ipaddress to get stream from
	-p, --port=PORT         : MANDATORY. port to listen to
	-s, --streamname=NAME   : MANDATORY. streamname to play
	-b, --backend=TYPE      : audio backend to use. possible values: alsa and pulseaudio. default is alsa
	-q, --quality=ID        : network quality indicator from 0 (low latency) to 4. default is 1
	-c, --channels=LIST     : channels from the stream to use. LIST is of form x,y,z,... default is to forward the stream as it is
	-o, --output=NAME       : Alsa output device name, as given by "aplay -L" output. using backend's default by default
	-d, --debug=LEVEL       : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)
	-h, --help              : display this message

About --channels option, a bit more tips:
* channels indexes are from 1 to 256 (as specified by VBAN specifications)
* you can repeat channels
* if you use in-existent channels, you will get silence but no error (as the stream may change at anytime)

Examples:

	vban_receptor -i IP -p PORT -s STREAMNAME -c1                   # keep only channel 1 and play out as mono
	vban_receptor -i IP -p PORT -s STREAMNAME -c1,1,1,1             # keep only channel 1 and play it out on 4 output channels (given that your output device is able to do it)
	vban_receptor -i IP -p PORT -s STREAMNAME -c2,41,125,7,1,45     # select some channels and play them out on 6 output channels (same comment)
