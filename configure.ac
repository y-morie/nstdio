#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nstdio], [0.9.0], [ykmorie@gmail.com])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_SRCDIR([include])
AC_CONFIG_HEADERS([config/config.h])
AC_CONFIG_MACRO_DIR(config)
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset select socket strdup])

AC_CONFIG_FILES([Makefile
                 example/Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT
