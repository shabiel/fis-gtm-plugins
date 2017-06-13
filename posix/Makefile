#################################################################
#								#
# Copyright (c) 2012-2015 Fidelity National Information 	#
# Services, Inc. and/or its subsidiaries. All rights reserved.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################
ifndef gtm_dist # verify that gtm_dist is defined
$(error $$gtm_dist not defined)
endif

DISTDIR = $(gtm_dist)
PLUGINDIR = $(DISTDIR)/plugin

CC = cc
LD = cc
CFLAGS = -c -O2 -I$(DISTDIR) -D_INCLUDE_XOPEN_SOURCE -D_INCLUDE_XOPEN_SOURCE_EXTENDED -D_INCLUDE_POSIX_SOURCE # regular build
# CFLAGS = -c -g -I$(DISTDIR) -D_INCLUDE_XOPEN_SOURCE -D_INCLUDE_XOPEN_SOURCE_EXTENDED -D_INCLUDE_POSIX_SOURCE # debug build
LDFLAGS =

# Uname output string to determine OS
UNAMESTR = $(shell uname -a)
ifneq (,$(findstring Linux,$(UNAMESTR)))
	FILEFLAG = -L
endif

# 64 bit system? 0 for yes!
BIT64 = $(shell file $(FILEFLAG) $(DISTDIR)/mumps | grep -q -E '64-bit|ELF-64'; echo $$?)

# 32 bit Linux systems don't support .so extension so we change it to .o later
SOEXT = so
# Is unicode support available on system?
UTF8MODE = 0

UTFLIBPATH = /usr/local/lib64:/usr/local/lib

ifeq ($(BIT64),0)
	CFLAGS += -D_FILE_OFFSET_BITS=64
	SIZEOFPTR = 8
else
	SIZEOFPTR = 4
endif

ifneq ($(wildcard $(DISTDIR)/utf8/mumps),)
	UTF8MODE = 1
endif

ifneq (,$(findstring AIX,$(UNAMESTR)))
	CFLAGS += -q64
	LDFLAGS += -G -bexpall -bnoentry -bh:4 -b64 -lc
	LDSHR = -brtl -G -bexpfull -bnoentry -b64
	UTFSTR = EN_US.UTF-8
endif

ifneq (,$(findstring HP-UX,$(UNAMESTR)))
	LD = ld
	CFLAGS += +Z +DD64
	LDFLAGS += -lelf -b
	LDSHR = -b
	UTFSTR = en_US.utf8
endif

ifneq (,$(findstring Linux,$(UNAMESTR)))
	CFLAGS += -fPIC
	LDFLAGS += -shared
	LDSHR = -shared
ifneq ($(BIT64),0)
		UTFLIBPATH = /usr/local/lib:/usr/lib:/usr/lib32
		SOEXT = o
endif
		UTFSTR = en_US.utf8
		UTFLIBPATH = /usr/local/lib64:/usr/local/lib:/usr/lib64:/usr/lib
endif

ifneq (,$(findstring Solaris,$(UNAMESTR)))
	CFLAGS += -KPIC -m64
	LDFLAGS += -G -64 -L /usr/lib/sparcv9 -L /usr/ucblib/sparcv9
	LDSHR = -G -64 -L /usr/lib/sparcv9 -L /usr/ucblib/sparcv9
	LDPRELOAD = LD_LIBRARY_PATH=/lib/64/:/usr/ucblib/sparcv9/
	UTFSTR = en_US.UTF-8
endif

LC_ALL = $(UTFSTR)
LC_CTYPE = $(UTFSTR)
gtm_chset = "UTF-8"
LIBPATH = $(UTFLIBPATH)
LD_LIBRARY_PATH = $(UTFLIBPATH)


all: 	libgtmposix.so gtmposix.xc

clean:
	-rm -f *.o *.so gtmposix.xc

gtmposix.o: gtmposix.c
	$(CC) $(CFLAGS) -o $@ gtmposix.c

gtmposix.xc: gtmposix.xc_proto
	-echo '$$PWD/libgtmposix.so' >$@
	-sed 's/sizeofptr/$(SIZEOFPTR)/g' <gtmposix.xc_proto >>$@

install: all
	mkdir -p $(PLUGINDIR)
	cp libgtmposix.so $(PLUGINDIR)/
	echo '$$gtm_dist/plugin/libgtmposix.so' >$(PLUGINDIR)/gtmposix.xc
	sed 's/sizeofptr/$(SIZEOFPTR)/g' <gtmposix.xc_proto >>$(PLUGINDIR)/gtmposix.xc
	chmod a-w $(PLUGINDIR)/libgtmposix.so  $(PLUGINDIR)/gtmposix.xc
	mkdir -p $(PLUGINDIR)/r
	mkdir -p $(PLUGINDIR)/o
	cp _POSIX.m $(PLUGINDIR)/r/
	chmod a-w $(PLUGINDIR)/r/_POSIX.m
	$(DISTDIR)/mumps -object=$(PLUGINDIR)/o/_POSIX.o $(PLUGINDIR)/r/_POSIX.m
ifneq ($(SOEXT),o)
	$(LD) $(LDSHR) -o $(PLUGINDIR)/o/_POSIX.$(SOEXT) $(PLUGINDIR)/o/_POSIX.o
	chmod a-w $(PLUGINDIR)/o/_POSIX.$(SOEXT)
	rm $(PLUGINDIR)/o/_POSIX.o
else
	chmod a-w $(PLUGINDIR)/o/_POSIX.o
endif
ifeq ($(UTF8MODE),1)
	mkdir -p $(PLUGINDIR)/o/utf8;
	$(DISTDIR)/utf8/mumps -object=$(PLUGINDIR)/o/utf8/_POSIX.o $(PLUGINDIR)/r/_POSIX.m
ifneq ($(SOEXT),o)
	$(LD) $(LDSHR) -o $(PLUGINDIR)/o/utf8/_POSIX.$(SOEXT) $(PLUGINDIR)/o/utf8/_POSIX.o
	chmod a-w $(PLUGINDIR)/o/utf8/_POSIX.$(SOEXT)
	rm $(PLUGINDIR)/o/utf8/_POSIX.o
else
	chmod a-w  $(PLUGINDIR)/o/utf8/_POSIX.o
endif
endif

libgtmposix.so: gtmposix.o
	$(LD) $(LDFLAGS) -o $@ gtmposix.o

test:	all
ifneq ($(wildcard $(DISTDIR)/libgtmutil.$(SOEXT)),)
	-($(LDPRELOAD) gtmroutines=". $(DISTDIR)/libgtmutil.$(SOEXT)" GTMXC_gtmposix=$(PWD)/gtmposix.xc $(DISTDIR)/mumps -run posixtest)
else
	-($(LDPRELOAD) gtmroutines=". $(DISTDIR)" GTMXC_gtmposix=$(PWD)/gtmposix.xc $(DISTDIR)/mumps -run posixtest)
endif

uninstall:
	-(cd $(PLUGINDIR) ; rm -f libgtmposix.$(SOEXT) gtmposix.xc r/_POSIX.m o/_POSIX.$(SOEXT) o/utf8/_POSIX.$(SOEXT))
