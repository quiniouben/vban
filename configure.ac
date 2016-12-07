#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([vban_receptor], [v0.8.0], [quiniouben@yahoo.fr])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_LANG(C)

# Manage conditional alsa enabling
AC_ARG_ENABLE([alsa],
[  --enable-alsa    Turn on alsa backend ],
[case "${enableval}" in
  yes) alsa=true ;;
  no)  alsa=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-alsa. default is yes]) ;;
esac],[alsa=true])
AM_CONDITIONAL([ALSA], [test x$alsa = xtrue])

AM_COND_IF([ALSA],
	[AC_CHECK_HEADERS([alsa/asoundlib.h], [], [AC_MSG_ERROR(Missing ALSA headers)])
	AC_CHECK_LIB([asound], [snd_pcm_open], [], [AC_MSG_ERROR(Missing ALSA library)])],
	[:]
)

# Manage conditional pulseaudio enabling
AC_ARG_ENABLE([pulseaudio],
[  --enable-pulseaudio    Turn on pulseaudio backend ],
[case "${enableval}" in
  yes) pulseaudio=true ;;
  no)  pulseaudio=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-pulseaudio. default is yes]) ;;
esac],[pulseaudio=true])
AM_CONDITIONAL([PULSEAUDIO], [test x$pulseaudio = xtrue])

AM_COND_IF([PULSEAUDIO],
	[AC_CHECK_HEADERS([pulse/simple.h], [], [AC_MSG_ERROR(Missing pulseaudio headers)])
	AC_CHECK_HEADERS([pulse/error.h], [], [AC_MSG_ERROR(Missing pulseaudio headers)])
	AC_CHECK_LIB([pulse-simple], [pa_simple_new], [], [AC_MSG_ERROR(Missing pulseaudio library)])
	AC_CHECK_LIB([pulse], [pa_strerror], [], [AC_MSG_ERROR(Missing pulseaudio library)])],
	[:]
)

# check that at least one backend is compiled
AM_CONDITIONAL([HAVE_BACKEND], [test x$pulseaudio = xtrue] || [test x$alsa = xtrue])
AM_COND_IF([HAVE_BACKEND],[:], [AC_MSG_ERROR(No audio backend compiled)])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h netinet/in.h stddef.h stdlib.h string.h sys/socket.h unistd.h], [], [AC_MSG_ERROR(Missing some system headers)])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([inet_ntoa memset socket strerror], [], [AC_MSG_ERROR(Missing some system functions)])

AC_OUTPUT(Makefile src/Makefile)
