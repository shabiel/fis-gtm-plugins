# I hate writing Makefiles!
ifndef gtm_dist
  $(error $$gtm_dist not defined)
endif

DISTDIR = $(gtm_dist)
PLUGINDIR = $(DISTDIR)/plugin

CC = cc
LD = cc
CFLAGS = -Wall -O3 -I$(DISTDIR) -fPIC
LDFLAGS = -shared -lcurl

all:	libcurl.so libcurl.xc
libcurl.so:	libcurl.o
	$(LD) -o $@ $^ $(LDFLAGS)
libcurl.xc:	libcurl.xc_proto
	echo './libcurl.so' > $@
	cat $^ >> $@

debug:	CFLAGS += -O0 -g
debug:	all

install: all
	cp libcurl.so $(PLUGINDIR)
	echo '$$gtm_dist/plugin/libcurl.so' > $(PLUGINDIR)/libcurl.xc
	cat libcurl.xc_proto >> $(PLUGINDIR)/libcurl.xc
	chmod a-w $(PLUGINDIR)/libcurl.so $(PLUGINDIR)/libcurl.xc
	echo 'Put this in your env file: '
	echo 'export GTMXC_libcurl="$(PLUGINDIR)/libcurl.xc"'

test: debug compile
	-@(gtmroutines=r GTMXC_libcurl=$(PWD)/libcurl.xc $(DISTDIR)/mumps -run libcurlPluginTests)
	-@(echo '')

.PHONY: compile
compile:
	@cd r ; \
	$(DISTDIR)/mumps -nowarning _ut.m ; \
	$(DISTDIR)/mumps -nowarning _ut1.m ; \
	$(DISTDIR)/mumps -nowarning libcurlPluginTests.m

.PHONY: clean
clean:
	rm -f libcurl.o libcurl.so libcurl.xc r/*.o
