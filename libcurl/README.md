# README
## Introduction
This C code provides an interface between YottaDB/GTM and libcurl. I developed
it because it was getting to be too hard to maintain a performant HTTP library
in M. I specifically needed it to provide all the newest features of TLS support.

The following features in libcurl have been implemented:

 * All HTTP verbs are supported
 * TLS supported - normal; with client certificates (w or w/o passwords); and with addition CA bundles. All TLS versions that curl supports are supported.
 * HTTP Basic Auth support; none others right now.
 * Can send in different mime types
 * Can adjust full timeout (seconds) and connect timeout (milliseconds)
 * Can get back output headers
 * Ability to reuse the same connection(s) for multiple HTTP requests

## Download
The source code needs to be compiled and installed into an existing GT.M/YDB
instance. Right now, you just need to download the repository

## Install
After downloading the software, you need to set your `gtm_dist` environment
variable and type `make install` as root (or can do `sudo -E make install`)
inside the libcurl directory. Here's some sample output:

``` 
saichiko:libcurl sam$ sudo -E make install
cc -Wall -O3 -I/usr/local/lib/yottadb-r123-x86_64-debug/ -fPIC   -c -o libcurl_ydb_wrapper.o libcurl_ydb_wrapper.c
cc -o libcurl_ydb_wrapper.so libcurl_ydb_wrapper.o -shared -lcurl
echo './libcurl_ydb_wrapper.so' > libcurl_ydb_wrapper.xc
cat libcurl_ydb_wrapper.xc_proto >> libcurl_ydb_wrapper.xc
cp libcurl_ydb_wrapper.so /usr/local/lib/yottadb-r123-x86_64-debug//plugin
echo '$gtm_dist/plugin/libcurl_ydb_wrapper.so' > /usr/local/lib/yottadb-r123-x86_64-debug//plugin/curl_ydb_wrapper.xc
cat libcurl_ydb_wrapper.xc_proto >> /usr/local/lib/yottadb-r123-x86_64-debug//plugin/libcurl_ydb_wrapper.xc
chmod a-w /usr/local/lib/yottadb-r123-x86_64-debug//plugin/libcurl_ydb_wrapper.so /usr/local/lib/yottadb-r123-x86_64-debug//plugin/libcurl_ydb_wrapper.xc
echo 'Put this in your env file: '
Put this in your env file:
echo 'export GTMXC_libcurl="/usr/local/lib/yottadb-r123-x86_64-debug//plugin/libcurl_ydb_wrapper.xc"'
export GTMXC_libcurl="/usr/local/lib/yottadb-r123-x86_64-debug//plugin/libcurl_ydb_wrapper.xc"
```

As it tells you, you need to add an environment variable to your GT.M/YDB
install: it's ``GTMXC_libcurl``. The above is a sample; the installer will give
you the actual value to put in.

## External Documentation
This project relies on libcurl (https://curl.haxx.se/). While you probably won't
need an extra documentation, you may need it if you want to add new features.

## M Entry Point Signatures
All the entry points can be invoked as extrinsics functions or as procedures.
For example, you can invoke `libcurl.init` as `do &libcurl.init` or `write
$&libcurl.init`. Unless indicated with a dot in front of a variable, all
parameters are free text values passed by value (except timeout, which is an
integer).

```
libcurl.curl(.httpStatusCode,.httpOutput,"HTTP VERB","URL","PAYLOAD","mime/type",timeout,.output headers)
libcurl.init()
libcurl.do(.httpCode,.httpOutput,"HTTP VERB","URL","PAYLOAD","mime/type",timeout,.output headers)
libcurl.cleanup()
libcurl.addHeader("header: text")
libcurl.auth("Basic","username:password")
libcurl.clientTLS("path to cert","path to key","key password","path to CA Bundle")
libcurl.conTimeoutMS(milliseconds)
```

## Usage
### Single GET/Multiple GET
The simplest use case is to use it to make a single GET request.

```
 n status s status=$&libcurl.curl(.httpStatus,.output,"GET","https://example.com")
```

`status` is the status of the C call, which is always 0 for successful transfers.
If you receive more than 1 MB, it will be 255, as GT.M/YDB cannot have a local
variable that is greater than 1 MB in length. Any status that's not zero will
throw an M error; so realistically, unless you set-up an error trap, you won't
capture non-zero statuses.

`httpStatus` will be 200 for successful connections; and `output` will contain
the output of your operation.  You do not have to make an extrinsic call and
keep a variable to keep track of the extrinsic output; you can call any
operation with a do command; just omit the dollar sign: `do
&libcurl.curl(...)`

If you will make multiple requests to the same host(s), you will be served well
(and honestly, that's the main reason I wrote the plugin) to initailize the
context, make the connections, and then close the context when you are done,
like this:

```
 n sss,zzz
 d &libcurl.init
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://example.com")
 d &libcurl.cleanup
```

Here's a realistic example, against the NLM RxNorm API:
```
 n sss,zzz,status
 d &libcurl.init
 s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/ndcstatus.json?ndc=00143314501")
 s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/drugs?name=cymbalta")
 s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/termtypes")
 s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/approximateTerm?term=zocor%2010%20mg&maxEntries=4")
 d &libcurl.cleanup
```

### PUT and POST
To do a PUT or a POST, you need to provide a payload, and make sure you use
PUT or POST as the HTTP verb. E.g.:

```
 n sss,zzz
 N PAYLOAD,RTN,H,RET
 N CRLF S CRLF=$C(13,10)
 N R S R=$R(123423421234)
 S PAYLOAD(1)="KBANTEST ; VEN/SMH - Test routine for Sam ;"_R
 S PAYLOAD(2)=" QUIT"
 S PAYLOAD=PAYLOAD(1)_CRLF_PAYLOAD(2)
 n status s status=$&libcurl.curl(.sss,.zzz,"POST","https://httpbin.org/post",PAYLOAD)
 QUIT
```

If you need to supply a mime type (as curl defaults to `application/x-www-form-urlencoded`), pass it in as the parameter after the payload. E.g.:

```
 n status s status=$&libcurl.curl(.sss,.zzz,"POST","https://httpbin.org/post",PAYLOAD,"application/json")
```

### Timeout
Full Timeout can be passed as the 7th parameter.

```
 n status s status=$&libcurl.curl(.sss,.zzz,"GET","https://example.com",,,5)
```

Connection timeout can be set after init:

```
 d &libcurl.init
 do
 . n $et s $et="set $ec="""""
 . d &libcurl.conTimeoutMS(1)
 . s status=$&libcurl.do(.sss,.zzz,"GET","https://example.com")
 d &libcurl.cleanup
```

`sss` will contain the [libcurl error as a negative number](https://curl.se/libcurl/c/libcurl-errors.html), and `zzz` will contain the texual description of the error.

### Receiving Response Headers
A reference variable can be added as the 8th parameter to receive the headers.
Note that they are all together as one string. You need to $piece them off by
$C(13,10) to get each individual header.

```
 n status s status=$&libcurl.curl(.sss,.zzz,"GET","https://example.com",,,5,.headers)
```

### Sending Custom Headers
To send custom headers, you need to use `.init` then `.addHeader`. The header
added needs to be the literal string you are sending as a header: so `DNT: 1`
not `"DNT",1`. E.g.:

```
 d &libcurl.init
 d &libcurl.addHeader("DNT: 1")
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://httpbin.org/headers",,,5,.headers)
 d &libcurl.cleanup
```

### Sending Basic HTTP Auth
You need to call `.auth` after `.init`.

```
 d &libcurl.init
 d &libcurl.auth("Basic","boo:foo")
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://httpbin.org/basic-auth/boo/foo",,"application/json")
 d &libcurl.cleanup
```

Other types of authentication are not supported right now.

### Using TLS Client Certificates or Adjusting Certificate Bundles
All certificate work is done via `.clientTLS`. As with other extension methods,
you need to call it after calling `.init`.

To authenticate against a server with a client certificate that does not have
a password, call `.clientTLS` with two parameters representing the certificate
and the key. E.g.:

```
 d &libcurl.init
 d &libcurl.clientTLS("/tmp/client.pem","/tmp/client.key")
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://prod.idrix.eu/secure/")
 d &libcurl.cleanup
```

If you need to supply a password to unlock the key, pass that as the third
parameter; e.g.:

```
 d &libcurl.clientTLS("/tmp/client.pem","/tmp/client.key","monkey1234")
```

You can add a CABundleFile as the 4th paramter. I haven't personally tested
that; so I don't know how will it work; but I thought it may be useful in
enterprise scenarios, where the certificates may all be self-generated.

To trust a specific certificate, using `.serverCA`, e.g.:

```
 d &libcurl.init
 d &libcurl.serverCA("/tmp/client.pem")
 d &libcurl.do(.httpStatus,.return,"GET","https://localhost:55730/ping")
 d &libcurl.cleanup
```

## Error Codes
The only way to trap errors is with an M error trap, as an error status runs the error trap. In the example below, `status` actaully NEVER gets set, but both `sss` and `zzz` do get set.

```
 d &libcurl.init
 do
 . n $et s $et="set $ec="""""
 . d &libcurl.conTimeoutMS(1)
 . s status=$&libcurl.do(.sss,.zzz,"GET","https://example.com")
 d &libcurl.cleanup
```

`sss` is either:

 * 0 - Everything is okay
 * 255 - Data or Header overflow error (greater than 1 MB in size)
 * -1 to -93: libcurl errors described here: https://curl.haxx.se/libcurl/c/libcurl-errors.html.

`zzz` is a texual description of `sss`.

## External Dependencies
This one is pretty obvious: This library uses libcurl; and runs on GT.M or YottaDB.

## Unit Tests
Running `make test` will run the Unit Tests. I am having difficulty with TCERT2
running consistently between platforms; it's disabled on Darwin for now but
enabled on Linux and Cygwin. Openssl may ask for a password when generating a
certificate even when supplied on the command line. In any case, if that test
fails for you, you can disable it by changing the @TEST to #TEST.

```
 ----------------------------- libcurlPluginTests -----------------------------
T1 - curl GET https://example.com...--------------------------  [OK]   94.516ms
T2 - curl GET https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json...
 -------------------------------------------------------------  [OK]  258.718ms
T3 - curl GET https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient...
 -------------------------------------------------------------  [OK]  149.587ms
T4 - init, cleanup runs w/o errors----------------------------  [OK]    1.784ms
T5 - do GET https://example.com...----------------------------  [OK]   70.650ms
TMI - Multiple GETs from Single Domain - Init-----------------  [OK]    1.559ms
TM1 - Multiple GETs from Single Domain - First.---------------  [OK]  163.308ms
TM2 - Multiple GETs from Single Domain - Second.--------------  [OK]   17.811ms
TM3 - Multiple GETs from Single Domain - Third.---------------  [OK]   12.817ms
TM4 - Multiple GETs from Single Domain - Fourth.--------------  [OK]   30.389ms
TM5 - Multiple GETs from Single Domain - Fifth.---------------  [OK]   15.744ms
TM6 - Mulitple GETs from Single Domain - Sixth.---------------  [OK]   64.836ms
TMC - Multiple GETs from Single Domain - Cleanup--------------  [OK]    1.371ms
TPAY - Test Payload...----------------------------------------  [OK]  228.651ms
TPAYMIME - Test Payload with mime type...---------------------  [OK]   95.221ms
TTO - do GET https://example.com with timeout...--------------  [OK]   71.703ms
THGET - do GET https://example.com with headers....-----------  [OK]   70.742ms
THSEND - do Send Custom Headers..-----------------------------  [OK]   80.103ms
TB100 - curl 100 bytes of binary data...----------------------  [OK]   86.745ms
TB1M - curl with >1M bytes of binary dataWeb Service return greater than GTM/YDB Max String Size 1048576---------------------  [OK]  825.846ms
TBAUTH - Basic authoriazation...------------------------------  [OK]  175.201ms
TCERT1 - Test TLS with a client certificate no key password

Generating a RSA private key
............................+++++
.................+++++
writing new private key to '/tmp/mycert.key'
-----
Generating a RSA private key
.......+++++
...............+++++
writing new private key to '/tmp/client.key'
-----
Signature ok
subject=/C=US/ST=Washington/L=Seattle/CN=www.smh101.com
Getting CA Private Key
..------------------------------------------------------------  [OK] 1886.440ms
```
## Future Work
At some point I have to stop working and make a release; there are a lot more
features that can be added. Here are my top two:

 * Add the ability to get more than 1 MB of data. I think the best way to do
   that is to save the data to a file and return the path of the file instead
	 of the data.
 * Depending on user demand, add other types of HTTP Authentication. I couldn't
   vouch for any of the others as I have not used them myself.
