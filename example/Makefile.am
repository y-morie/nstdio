#
# Makefile.am for NSTDIO
#

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/src/include
LDADD = $(top_builddir)/src/libnstdio.la $(top_builddir)/src/libppstream.la
noinst_PROGRAMS = sv cl pc fe node test-ping test-pingd test-pingc test-perfd test-perfc

sv_DEPENDENCIES = $(LDADD)
sv_SOURCES = test-sv.c nstdio.h ppstream.h

cl_DEPENDENCIES = $(LDADD)
cl_SOURCES = test-cl.c nstdio.h ppstream.h

pc_DEPENDENCIES = $(LDADD)
pc_SOURCES = test-pc.c nstdio.h ppstream.h

fe_DEPENDENCIES = $(LDADD)
fe_SOURCES = test-fe.c nstdio.h ppstream.h

node_DEPENDENCIES = $(LDADD)
node_SOURCES = test-node.c nstdio.h ppstream.h

test_ping_DEPENDENCIES = $(LDADD)
test_ping_SOURCES = test-ping.c nstdio.h ppstream.h

test_pingd_DEPENDENCIES = $(LDADD)
test_pingd_SOURCES = test-pingd.c nstdio.h ppstream.h

test_pingc_DEPENDENCIES = $(LDADD)
test_pingc_SOURCES = test-pingc.c nstdio.h ppstream.h

test_perfd_DEPENDENCIES = $(LDADD)
test_perfd_SOURCES = test-perfd.c nstdio.h ppstream.h

test_perfc_DEPENDENCIES = $(LDADD)
test_perfc_SOURCES = test-perfc.c nstdio.h ppstream.h


