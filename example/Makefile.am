#
# Makefile.am for NSTDIO
#

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/src/include
LDADD = $(top_builddir)/src/libnstdio.la $(top_builddir)/src/libppstream.la
noinst_PROGRAMS = sv cl pc fe node test-ping test-pingd test-pingc test-perfd test-perfc test-perfi test-perfl

sv_DEPENDENCIES = $(LDADD)
sv_SOURCES = test-sv.c 

cl_DEPENDENCIES = $(LDADD)
cl_SOURCES = test-cl.c

pc_DEPENDENCIES = $(LDADD)
pc_SOURCES = test-pc.c 

fe_DEPENDENCIES = $(LDADD)
fe_SOURCES = test-fe.c 

node_DEPENDENCIES = $(LDADD)
node_SOURCES = test-node.c 

test_ping_DEPENDENCIES = $(LDADD)
test_ping_SOURCES = test-ping.c 

test_pingd_DEPENDENCIES = $(LDADD)
test_pingd_SOURCES = test-pingd.c 

test_pingc_DEPENDENCIES = $(LDADD)
test_pingc_SOURCES = test-pingc.c 

test_perfd_DEPENDENCIES = $(LDADD)
test_perfd_SOURCES = test-perfd.c 

test_perfc_DEPENDENCIES = $(LDADD)
test_perfc_SOURCES = test-perfc.c 

test_perfi_DEPENDENCIES = $(LDADD)
test_perfi_SOURCES = test-perfi.c

test_perfl_DEPENDENCIES = $(LDADD)
test_perfl_SOURCES = test-perfl.c


