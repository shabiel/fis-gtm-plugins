This is the readme.txt file for the GTMJI plug-in.


OVERVIEW

GTMJI provides a mechanism to call-in to GTM from Java application
code, and to call out from GT.M to Java application code. GTMJI
requires a minimum GT.M release of V6.0-002 and is supported on Linux
on x86 and x86_64, AIX, and Solaris on SPARC.

GTMJI distribution consists of the following files and directories:

  - callin.m - a sample M program to test the operation of Java
    call-ins with GT.M.

  - callin.tab - the call-in table containing mappings for the call-in
    test.

  - callout.m - a sample M program to test the operation of Java
    call-outs with GT.M.

  - callout.tab.tpl - a template for the call-out table containing
    mappings for the call-out test; the template is missing the path to
    the libgtmm2j.so shared library.

  - ci_gateway.c - the source code for the call-in interface logic
    between Java and GT.M.

  - ci_gateway.h - the header file containing the prototypes of
    functions implemented in ci_gateway.c.

  - com/fis/gtm/ji/*.java - Java classes containing library-loading,
    call-scheduling, and termination logic for Java call-ins, and
    defining GTMJI wrapper types for argument processing on the Java
    side.

  - com/fis/test/*.java - sample Java classes for testing the operation
    of Java call-ins and call-outs with GT.M.

  - COPYING - the free / open source software (FOSS) license under
    which GTMJI is provided to you. See section LICENSE below for more
    information, especially if COPYING is missing when you receive
    GTMJI.

  - gtmji.h - the header file containing precompiled and global
    definitions common for both ci_gateway.c and xc_gateway.c.

  - Makefile - for use by GNU make to build, test, install, and
    uninstall the package.

  - xc_gateway.c - the source code for the call-out interface logic
    between GT.M and Java.

  - xc_gateway.h - the header file containing the prototypes of
    functions implemented in xc_gateway.c.


LICENSE

If you receive this plug-in integrated with a GT.M distribution, the
license for your GT.M distribution is also your license for this
plug-in.

In the event the package contains a COPYING file, that is your license
for this plug-in. Except as noted here, you must retain that file and
include it if you redistribute the package or a derivative work
thereof.

If you have a signed agreement providing you with GT.M under a
different license from that in the COPYING file, you may, at your
option, delete the COPYING file and use GTMJI as an integral part of
GT.M under the same terms as that of the GT.M license. If your GT.M
license permits redistribution, you may redistribute GTMJI integrated
with GT.M under the terms of that license.

Simple aggregation or bundling of this package with another for
distribution does not create a derivative work. To make clear the
distinction between this package and another with which it is
aggregated or bundled, it suffices to place each package in a separate
directory or folder when distributing the aggregate or bundle.

Should you receive this package not integrated with a GT.M
distribution, and missing a COPYING file, you may create a file called
COPYING from the GNU Affero General Public License Version 3 or later
(https://www.gnu.org/licenses/agpl.txt) and use this package under the
terms of that license.


INSTALLATION AND USAGE

Please refer to the GTMJI Technical Bulletin for the detailed
information on installation and usage procedures as well as testing and
examples.
