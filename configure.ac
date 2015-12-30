#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([vban_receptor], [v0.5], [quiniouben@yahoo.fr])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_LANG(C)

# Checks for libraries.
AC_CHECK_HEADERS([alsa/asoundlib.h], [], [AC_MSG_ERROR(Missing ALSA headers)])
AC_CHECK_LIB([asound], [snd_pcm_open], [], [AC_MSG_ERROR(Missing ALSA library)])

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
