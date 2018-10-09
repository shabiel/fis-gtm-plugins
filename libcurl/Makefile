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

all:	libcurl_ydb_wrapper.so libcurl_ydb_wrapper.xc
libcurl_ydb_wrapper.so:	libcurl_ydb_wrapper.o
	$(LD) -o $@ $^ $(LDFLAGS)
libcurl_ydb_wrapper.xc:	libcurl_ydb_wrapper.xc_proto
	echo './libcurl_ydb_wrapper.so' > $@
	cat $^ >> $@

debug:	CFLAGS += -O0 -g
debug:	all

install: all
	cp libcurl_ydb_wrapper.so $(PLUGINDIR)
	echo '$$gtm_dist/plugin/libcurl_ydb_wrapper.so' > $(PLUGINDIR)/curl_ydb_wrapper.xc
	cat libcurl_ydb_wrapper.xc_proto >> $(PLUGINDIR)/libcurl_ydb_wrapper.xc
	chmod a-w $(PLUGINDIR)/libcurl_ydb_wrapper.so $(PLUGINDIR)/libcurl_ydb_wrapper.xc
	echo 'Put this in your env file: '
	echo 'export GTMXC_libcurl="$(PLUGINDIR)/libcurl_ydb_wrapper.xc"'

test: debug compile
	-@(gtmroutines=r GTMXC_libcurl=$(PWD)/libcurl_ydb_wrapper.xc $(DISTDIR)/mumps -run libcurlPluginTests)
	-@(echo '')

.PHONY: compile
compile:
	@cd r ; \
	$(DISTDIR)/mumps -nowarning _ut.m ; \
	$(DISTDIR)/mumps -nowarning _ut1.m ; \
	$(DISTDIR)/mumps -nowarning libcurlPluginTests.m

.PHONY: clean
clean:
	rm -f libcurl_ydb_wrapper.o libcurl_ydb_wrapper.so libcurl_ydb_wrapper.xc r/*.o
