# I hate writing Makefiles!
ifndef gtm_dist
  $(error $$gtm_dist not defined)
endif

DISTDIR = $(gtm_dist)
PLUGINDIR = $(DISTDIR)/plugin

CC = cc
LD = cc
CFLAGS = -O3 -I$(DISTDIR) -fPIC
LDFLAGS = -shared -lcrypto

all:	libopenssl_ydb_wrapper.so openssl_ydb_wrapper.xc
libopenssl_ydb_wrapper.so:	openssl_ydb_wrapper.o
	$(LD) -o $@ $^ $(LDFLAGS)
openssl_ydb_wrapper.xc:	openssl_ydb_wrapper.xc_proto
	echo './libopenssl_ydb_wrapper.so' > $@
	cat $^ >> $@

debug:	CFLAGS += -g
debug:	all

install: all
	cp libopenssl_ydb_wrapper.so $(PLUGINDIR)
	echo '$$gtm_dist/plugin/libopenssl_ydb_wrapper.so' > $(PLUGINDIR)/openssl_ydb_wrapper.xc
	cat openssl_ydb_wrapper.xc_proto >> $(PLUGINDIR)/openssl_ydb_wrapper.xc
	chmod a-w $(PLUGINDIR)/libopenssl_ydb_wrapper.so $(PLUGINDIR)/openssl_ydb_wrapper.xc
	echo 'Put this in your env file: '
	echo 'export GTMXC_openssl="$(PLUGINDIR)/openssl_ydb_wrapper.xc"'

test: all compile
	-@(gtmroutines=r GTMXC_openssl=$(PWD)/openssl_ydb_wrapper.xc $(DISTDIR)/mumps -run opensslPluginTests)

.PHONY: compile
compile:
	@cd r ; \
	$(DISTDIR)/mumps -nowarning _ut.m ; \
	$(DISTDIR)/mumps -nowarning _ut1.m ; \
	$(DISTDIR)/mumps -nowarning opensslPluginTests.m

.PHONY: clean
clean:
	rm -f openssl_ydb_wrapper.o libopenssl_ydb_wrapper.so openssl_ydb_wrapper.xc r/*.o
