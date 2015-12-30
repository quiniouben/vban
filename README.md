vban_receptor - Linux command-line VBAN to alsa player
======================================================

&copy; 2015 Benoît Quiniou - quiniouben[at]yahoo(.)fr

vban_receptor is an open-source implementation of VBAN protocol.
It currently takes the form of a Linux command-line player to play a VBAN audio stream to alsa local audio device.
VBAN is a simple audio over UDP protocol proposed by VB-Audio, see [VBAN Audio webpage](http://vb-audio.pagesperso-orange.fr/Voicemeeter/vban.htm).

Compilation and installation
----------------------------

vban_receptor uses ALSA, therefore you need to have ALSA library and headers available on your platform.
In standard Linux distributions, ALSA library package is usually named libasound(X) and header files come in package named libasound(X)-dev.

vban_receptor is distributed with autotools build scripts, therefore, to build, you need to invoke:
	$ ./autogen.sh              # probably only once for ever
	$ ./configure               # with or without options
	$ make                      # with or without options

To install, simply invoke:
	# make install

Usage
-----

Invoking vban_receptor without any parameter will give hints on how to use it :

	Usage: vban_receptor [OPTIONS]...

	-i, --ipaddress=IP    : ipaddress to get stream from
	-p, --port=PORT       : port to listen to
	-s, --streamname=NAME : streamname to play
	-q, --quality=ID      : network quality indicator from 0 (low latency) to 4. default is 1
	-h, --help            : display this message


