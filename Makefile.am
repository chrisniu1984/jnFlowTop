AUTOMAKE_OPTIONS=foreign
CLEANFILES = autoscan.log config.log
DISTCLEANFILES = aclocal.m4 configure depcomp install-sh missing Makefile.in

SUBDIRS = jnsdk

INCLUDES =-I./ -I./jnsdk
AM_CFLAGS = -Wall

bin_PROGRAMS = jnFlowTop

jnFlowTop_SOURCES =	main.c \
					session.c \
					process.c \
			 		inode.c \
			 		core.c

jnFlowTop_LDADD =	$(top_srcdir)/jnsdk/libjnsdk.a \
					-lpthread

clean-local:
	rm -rf autom4te.cache
