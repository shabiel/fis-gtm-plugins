#################################################################
#								#
#	Copyright 2013, 2014 Fidelity Information Services, Inc	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

DEBUG = 0
ifdef gtmji_make_debug
DEBUG = 1
endif

# Verify that gtm_dist is defined.
ifndef gtm_dist
$(error $$gtm_dist not defined!)
endif

# Verify that JAVA_HOME is defined.
ifndef JAVA_HOME
$(error $$JAVA_HOME not defined!)
endif

# Verify that JAVA_SO_HOME is defined.
ifndef JAVA_SO_HOME
$(error $$JAVA_SO_HOME not defined!)
endif

# Verify that JVM_SO_HOME is defined.
ifndef JVM_SO_HOME
$(error $$JVM_SO_HOME not defined!)
endif

DISTDIR = $(gtm_dist)
PLUGINDIR = $(DISTDIR)/plugin
JAVADIR = $(JAVA_HOME)
JAVASODIR = $(JAVA_SO_HOME)
JVMSODIR = $(JVM_SO_HOME)
CURDIR = $(PWD)

# Uname output string to determine OS.
UNAMESTR = $(shell uname -a)

ifneq (,$(findstring Linux,$(UNAMESTR)))
	FILEFLAG = -L
endif

# 64 bit system? 0 for yes!
BIT64 = $(shell file $(FILEFLAG) $(DISTDIR)/mumps | grep -q -E '64-bit|ELF-64'; echo $$?)

LDSHR =
JAVA_OPTS = -Xss4M

# Is unicode support available on system?
UTF8MODE = 0
UTFLIBPATH = /usr/local/lib64:/usr/local/lib:/usr/lib64:/usr/lib
UTFPLUGINDIR = $(DISTDIR)/utf8/plugin
UTFDISTDIR = $(gtm_dist)/utf8
ifneq ($(wildcard $(DISTDIR)/utf8/mumps),)
	UTF8MODE = 1
endif

# Linux
ifneq (,$(findstring Linux,$(UNAMESTR)))
	CI_CC = cc
	XC_CC = cc
	CI_CFLAGS = -fPIC -Wvla -Wall
	XC_CFLAGS = -fPIC -Wvla -Wall
	INCLUDE_DIR = linux
	LDSHR = -shared
	LIB_DIR =
ifneq ($(BIT64),0)
	UTFLIBPATH = /usr/local/lib:/usr/lib:/usr/lib32
endif
	UTFSTR = en_US.utf8
endif

# Solaris
ifneq (,$(findstring Solaris,$(UNAMESTR)))
	CI_CC = cc
	XC_CC = cc
	CI_CFLAGS = -m64 -fPIC -shared
	XC_CFLAGS = -m64 -fPIC -shared
	INCLUDE_DIR = solaris
	LDSHR = -shared
	UTFSTR = en_US.UTF-8
	JAVA_OPTS += -d64
endif

# AIX
ifneq (,$(findstring AIX,$(UNAMESTR)))
	CI_CC = cc_r
	XC_CC = cc
	CI_CFLAGS = -q64 -G -qarch=ppc -D_THREAD_SAFE -qthreaded
	XC_CFLAGS = -q64 -brtl -G -D_THREAD_SAFE -qthreaded -bexpall -qlanglvl=extc99
	INCLUDE_DIR = aix
	UTFSTR = EN_US.UTF-8
	JAVA_OPTS += -Xmso4M
endif

CI_CFLAGS += -I$(DISTDIR) -I$(JAVADIR)/include -I$(JAVADIR)/include/$(INCLUDE_DIR)
CI_LDFLAGS = -lpthread
XC_CFLAGS += -I$(DISTDIR) -I$(JAVADIR)/include -I$(JAVADIR)/include/$(INCLUDE_DIR)
XC_LDFLAGS = -lpthread -L$(JAVASODIR) -ljava -L$(JVMSODIR) -ljvm
LDSHR += -o

ifneq ($(DEBUG),0)
	CI_CFLAGS += -g
	XC_CFLAGS += -g
endif

CI_CFLAGS += -DGTM_PTHREAD
XC_CFLAGS += -DGTM_PTHREAD

ENV_BASE = LIBPATH="$(UTFLIBPATH):$(DISTDIR):$(JAVASODIR):$(JVMSODIR)" \
	LD_LIBRARY_PATH="$(UTFLIBPATH):$(JAVASODIR):$(JVMSODIR)" \
	GTMCI="$(CURDIR)/callin.tab" \
	GTMXC_gtmm2j="$(CURDIR)/callout.tab" \
	gtmroutines=". $(DISTDIR)"

PLUGINENV = $(ENV_BASE) \
	GTMXC_classpath="$(CURDIR):$(PLUGINDIR)/gtmji.jar"

TESTENV = $(ENV_BASE) \
	GTMXC_classpath="$(CURDIR):$(CURDIR)/gtmji.jar"

PLUGIN_FILES = $(PLUGINDIR)/libgtmj2m.so $(PLUGINDIR)/libgtmm2j.so $(PLUGINDIR)/gtmji.jar

ENV_UTF = LC_ALL="" LC_CTYPE="$(UTFSTR)" gtm_chset="UTF-8" gtm_dist="$(UTFDISTDIR)"
UTFPLUGINENV = $(PLUGINENV) $(ENV_UTF)
UTFTESTENV = $(TESTENV) $(ENV_UTF)
ifeq ($(UTF8MODE),1)
	PLUGIN_FILES += $(UTFPLUGINDIR)/libgtmj2m.so $(UTFPLUGINDIR)/libgtmm2j.so $(UTFPLUGINDIR)/gtmji.jar
endif

JAVA_FILES = $(wildcard com/fis/gtm/ji/*.java)

libgtmj2m.so: ci_gateway.c ci_gateway.h
	@echo ; echo "Compiling libgtmj2m.so..."
	$(CI_CC) $(CI_CFLAGS) $< $(CI_LDFLAGS) $(LDSHR) libgtmj2m.so

libgtmm2j.so: xc_gateway.c xc_gateway.h
	@echo ; echo "Compiling libgtmm2j.so..."
	$(XC_CC) $(XC_CFLAGS) $< $(XC_LDFLAGS) $(LDSHR) libgtmm2j.so

gtmji.jar: $(JAVA_FILES)
	@echo ; echo "Compiling gtmji.jar..."
	$(JAVADIR)/bin/javac $^
	$(JAVADIR)/bin/jar cvf gtmji.jar com/fis/gtm/ji/*.class

all: libgtmj2m.so libgtmm2j.so gtmji.jar

$(PLUGINDIR)/libgtmj2m.so: libgtmj2m.so
	@echo ; echo "Installing libgtmj2m.so to $(PLUGINDIR)..."
	mkdir -p $(PLUGINDIR)
	cp -f libgtmj2m.so $(PLUGINDIR)/
	chmod a-w $(PLUGINDIR)/libgtmj2m.so

$(PLUGINDIR)/libgtmm2j.so: libgtmm2j.so
	@echo ; echo "Installing libgtmm2j.so to $(PLUGINDIR)..."
	mkdir -p $(PLUGINDIR)
	cp -f libgtmm2j.so $(PLUGINDIR)/
	chmod a-w $(PLUGINDIR)/libgtmm2j.so

$(PLUGINDIR)/gtmji.jar: gtmji.jar
	@echo ; echo "Installing gtmji.jar to $(PLUGINDIR)..."
	mkdir -p $(PLUGINDIR)
	cp -f gtmji.jar $(PLUGINDIR)/
	chmod a-w $(PLUGINDIR)/gtmji.jar

$(UTFPLUGINDIR)/libgtmj2m.so: libgtmj2m.so
	@echo ; echo "Installing libgtmj2m.so to $(UTFPLUGINDIR)..."
	mkdir -p $(UTFPLUGINDIR)
	cp -f libgtmj2m.so $(UTFPLUGINDIR)/
	chmod a-w $(UTFPLUGINDIR)/libgtmj2m.so

$(UTFPLUGINDIR)/libgtmm2j.so: libgtmm2j.so
	@echo ; echo "Installing libgtmm2j.so to $(UTFPLUGINDIR)..."
	mkdir -p $(UTFPLUGINDIR)
	cp -f libgtmm2j.so $(UTFPLUGINDIR)/
	chmod a-w $(UTFPLUGINDIR)/libgtmm2j.so

$(UTFPLUGINDIR)/gtmji.jar: gtmji.jar
	@echo ; echo "Installing gtmji.jar to $(UTFPLUGINDIR)..."
	mkdir -p $(UTFPLUGINDIR)
	cp -f gtmji.jar $(UTFPLUGINDIR)/
	chmod a-w $(UTFPLUGINDIR)/gtmji.jar

test: all
	@rm -f callin.o
	@rm -f callout.o
	@echo ; echo "Testing call-ins in M mode..."
	$(JAVADIR)/bin/javac -classpath $(CURDIR):$(CURDIR)/gtmji.jar com/fis/test/CallinTest.java
	$(TESTENV) $(JAVADIR)/bin/java $(JAVA_OPTS) -classpath $(CURDIR):$(CURDIR)/gtmji.jar -Djava.library.path=$(CURDIR) com.fis.test.CallinTest
	@echo ; echo "Testing call-outs in M mode..."
	$(JAVADIR)/bin/javac -classpath $(CURDIR):$(CURDIR)/gtmji.jar com/fis/test/CalloutTest.java
	echo "$(CURDIR)/libgtmm2j.so" > callout.tab
	cat callout.tab.tpl >> callout.tab
	$(TESTENV) LD_PRELOAD=$(JVMSODIR)/libjsig.so $(DISTDIR)/mumps -run callout
ifeq ($(UTF8MODE),1)
	@rm -f callin.o
	@rm -f callout.o
	@echo ; echo "Testing call-ins in UTF-8 mode..."
	$(UTFTESTENV) $(JAVADIR)/bin/java $(JAVA_OPTS) -classpath $(CURDIR):$(CURDIR)/gtmji.jar -Djava.library.path=$(CURDIR) com.fis.test.CallinTest
	@echo ; echo "Testing call-outs in UTF-8 mode..."
	$(UTFTESTENV) LD_PRELOAD=$(JVMSODIR)/libjsig.so $(UTFDISTDIR)/mumps -run callout
endif

clean:
	@echo ; echo "Removing generated files..."
	rm -f libgtm*.so
	rm -f gtmji.jar
	rm -f com/fis/gtm/ji/*.class
	rm -f com/fis/test/*.class
	rm -f *.o
	rm -f callout.tab

install: $(PLUGIN_FILES)

install-test: $(PLUGIN_FILES)
	@rm -f callin.o
	@rm -f callout.o
	@echo ; echo "Testing call-ins in M mode..."
	$(JAVADIR)/bin/javac -classpath $(CURDIR):$(PLUGINDIR)/gtmji.jar com/fis/test/CallinTest.java
	$(PLUGINENV) $(JAVADIR)/bin/java $(JAVA_OPTS) -classpath $(CURDIR):$(PLUGINDIR)/gtmji.jar -Djava.library.path=$(PLUGINDIR) com.fis.test.CallinTest
	@echo ; echo "Testing call-outs in M mode..."
	$(JAVADIR)/bin/javac -classpath $(CURDIR):$(PLUGINDIR)/gtmji.jar com/fis/test/CalloutTest.java
	echo "$(CURDIR)/libgtmm2j.so" > callout.tab
	cat callout.tab.tpl >> callout.tab
	$(PLUGINENV) LD_PRELOAD=$(JVMSODIR)/libjsig.so $(DISTDIR)/mumps -run callout
ifeq ($(UTF8MODE),1)
	@rm -f callin.o
	@rm -f callout.o
	@echo ; echo "Testing call-ins in UTF-8 mode..."
	$(JAVADIR)/bin/javac -classpath $(CURDIR):$(UTFPLUGINDIR)/gtmji.jar com/fis/test/CallinTest.java
	$(UTFPLUGINENV) $(JAVADIR)/bin/java $(JAVA_OPTS) -classpath $(CURDIR):$(UTFPLUGINDIR)/gtmji.jar -Djava.library.path=$(UTFPLUGINDIR) com.fis.test.CallinTest
	@echo ; echo "Testing call-outs in UTF-8 mode..."
	$(JAVADIR)/bin/javac -classpath $(CURDIR):$(UTFPLUGINDIR)/gtmji.jar com/fis/test/CalloutTest.java
	$(UTFPLUGINENV) LD_PRELOAD=$(JVMSODIR)/libjsig.so $(UTFDISTDIR)/mumps -run callout
endif

uninstall:
	@echo ; echo "Uninstalling plug-in from $(PLUGINDIR)..."
	rm -f $(PLUGINDIR)/libgtmj2m.so
	rm -f $(PLUGINDIR)/libgtmm2j.so
	rm -f $(PLUGINDIR)/gtmji.jar
ifeq ($(UTF8MODE),1)
	@echo ; echo "Uninstalling plug-in from $(UTFPLUGINDIR)..."
	rm -f $(UTFPLUGINDIR)/libgtmj2m.so
	rm -f $(UTFPLUGINDIR)/libgtmm2j.so
	rm -f $(UTFPLUGINDIR)/gtmji.jar
endif
