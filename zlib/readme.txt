OVERVIEW

gtmzlib is a simple plugin to allow GT.M application code to use zlib
(http://zlib.net) to compress and uncompress string data.  gtmzlib is
itself provided to you under the terms of the same license that GT.M
itself is provided to you.  gtmzlib is simply a wrapper for zlib, and
does not include zlib, which you must indepentently procure and
install.

gtmzlib consists of an M module %ZLIB, which in turn calls zlib, to
provide the following entryrefs:

$$compress2^%ZLIB(origstr,.compstr,level)
$$uncompress^%ZLIB(compstr,.uncompstr)
$$version^%ZLIB
$$zlibversion^%ZLIB

compress2 compresses origstr and returns the compressed string in
compstr; level is the string passed to zlib as the compression level
(-1 through 9, defaulting to -1 if not specified, which in turn
instructs zlib to use its default compression).  uncompress expands
compstr and provides the result in uncompstr.  Both compress2 and
uncompress return the status returned by the underlying zlib code; 0
for a normal return.  For example:

GTM>set a="The quick brown fox"

GTM>set s=$$compress2^%ZLIB(a,.b)

GTM>zwrite
a="The quick brown fox"
b="x"_$C(156,11)_"�HU(,�L�VH*�/�SH˯"_$C(0,0)_"E."_$C(7,20)
s=0

GTM>set s=$$uncompress^%ZLIB(b,.c)

GTM>zwrite
a="The quick brown fox"
b="x"_$C(156,11)_"�HU(,�L�VH*�/�SH˯"_$C(0,0)_"E."_$C(7,20)
c="The quick brown fox"
s=0

GTM>

zlibversion returns the version of the zlib library, version returns
the version of the plugin, which is a timestamp in $horolog format.
For example:

GTM>set z=$$zlibversion^%ZLIB

GTM>set p=$$version^%ZLIB

GTM>zwrite
p="62508,48729"
z="1.2.3.4"

GTM>


INSTALLATION

gtmzlib consists of three code files - gtmzlib.c, gtmzlib.xc, and
_ZLIB.m - and one readme.txt.  It may also contain a COPYING file
describing the terms of the license under which it is provided.

Compile the gtmzlib.c source file to produce a libgtmzlib.so shared
library that can be called from GT.M code.  The following example is
from Linux and assumes that a 64-bit version of GT.M V5.5-000 for the
x86 architecture is installed in /usr/lib/fis-gtm/V5.5-000_x86_64;
refer to the GT.M Programmers Guide for the commands on your platform
and adjust as needed for your specific directory structure.

$ ls -l
total 28
-rw-r--r-- 1 gtmuser gtc 1358 2012-02-21 16:56 gtmzlib.c
-rw-r--r-- 1 gtmuser gtc  282 2012-02-21 16:56 gtmzlib.xc
-rw-r--r-- 1 gtmuser gtc 1471 2012-02-21 17:17 _ZLIB.m
$ gcc -c -fPIC -I/usr/lib/fis-gtm/V5.5-000_x86_64 gtmzlib.c
$ gcc -o libgtmzlib.so -shared gtmzlib.o
$ ls -l
total 28
-rw-r--r-- 1 gtmuser gtc 1358 2012-02-21 16:56 gtmzlib.c
-rw-r--r-- 1 gtmuser gtc 1976 2012-02-21 18:00 gtmzlib.o
-rw-r--r-- 1 gtmuser gtc  282 2012-02-21 16:56 gtmzlib.xc
-rwxr-xr-x 1 gtmuser gtc 7997 2012-02-21 18:00 libgtmzlib.so
-rw-r--r-- 1 gtmuser gtc 1471 2012-02-21 17:17 _ZLIB.m
$ 

Copy the gtmzlib.xc and libgtmzlib.so files to the plugin subdirectory
of your GT.M directory; in this example,
/usr/lib/fis-gtm/V5.5-000_z86_64/plugin (you will need to run this
command as root, or other administrative userid needed to write into a
sub-directory of the GT.M installation):

$ sudo cp gtmzlib.xc libgtmzlib.so /usr/lib/fis-gtm/V5.5-000_x86_64/plugin/

Copy the _ZLIB.m file to the /usr/lib/fis-gtm/V5.5-000_x86_64/plugin/r
and compile it with an M-mode object module in
/usr/lib/fis-gtm/V5.5-000_x86_64/plugin/o and a UTF-8 mode object
module in /usr/lib/fis-gtm/V5.5-000_x86_64/plugin/o/utf8.

$ find /usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin -iname \*zlib\*
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/libgtmzlib.so
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/r/_ZLIB.m
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/o/utf8/_ZLIB.o
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/o/_ZLIB.o
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/gtmzlib.xc
$

If your GT.M platform supports shared libraries, you can replace the
.o object files with shared libraries for more efficient memory usage.

$ find /usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin -iname \*zlib\*
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/libgtmzlib.so
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/r/_ZLIB.m
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/o/utf8/_ZLIB.o
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/o/_ZLIB.o
/usr/lib/fis-gtm/V5.5-FT03_x86_64/plugin/gtmzlib.xc
$

When you run GT.M, if your system has not been configured to
automatically locate the zlib shared library on your system, you will
need to do that (see man ldconfig on Linux) or explicitly preload the
library (e.g., with the LD_PRELOAD environment variable on Linux; the
location of libz.so below is from Ubuntu Linux 11.10).  Effective
V5.5-000, the gtmprofile and gtm scripts automatically provide the
environment variables needed to access any plugin installed following
these instructions.  For older releases of GT.M, you will need to
provide the GTMXC_gtmzlib environment variable, and ensure that the
_ZLIB routine is found by $zroutines.

$ LD_PRELOAD=/lib/x86_64-linux-gnu/libz.so.1.2.3.4 /usr/lib/fis-gtm/V5.5-000_x86_64/gtm

GTM>set a="The quick brown fox jumps over the lazy dog"

GTM>set s=$$compress2^%ZLIB(a,.b,9)

GTM>set t=$$uncompress^%ZLIB(b,.c)

GTM>zwrite
a="The quick brown fox jumps over the lazy dog"
b="x�"_$C(11)_"�HU(,�L�VH*�/�SH˯P�*�-(V�/K-R("_$C(1)_"J�$VU*���"_$C(3,0)_"[�"_$C(15)_"�"
c="The quick brown fox jumps over the lazy dog"
s=0
t=0

GTM>


PLEASE NOTE

zlib is not FIS software and is not supported as part of FIS GT.M
support.  FIS strongly encourages you to ensure that you have
appropriate support for software that you rely on.

The pre-allocation for return strings in gtmzlib.xc, whether
compressed or uncompressed, allows for strings up to 1048576 bytes
(1MB) which is the longest string value currently supported by GT.M.
Extensive use of gtmzlib may therefore result in frequent garbage
collection.  If your application is guaranteed to use strings only
smaller than 1MB, you can reduce this number accordingly.

The GT.M interface to call out to C libraries is a low-level interface
designed for use by programmers rather than end-users.  Misuse, abuse
and bugs can result in applications that are fragile, hard to
troubleshoot and with security vulnerabilities.
