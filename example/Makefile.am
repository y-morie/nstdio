#
# Makefile.am for NSTDIO
#

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/src/include
LDADD = $(top_builddir)/src/libnstdio.la
noinst_PROGRAMS = sv cl test-ping test-pingd test-pingc

sv_DEPENDENCIES = $(LDADD)
sv_SOURCES = test-sv.c nstdio.h

cl_DEPENDENCIES = $(LDADD)
cl_SOURCES = test-cl.c nstdio.h

test_ping_DEPENDENCIES = $(LDADD)
test_ping_SOURCES = test-ping.c nstdio.h

test_pingd_DEPENDENCIES = $(LDADD)
test_pingd_SOURCES = test-pingd.c nstdio.h

test_pingc_DEPENDENCIES = $(LDADD)
test_pingc_SOURCES = test-pingc.c nstdio.h

