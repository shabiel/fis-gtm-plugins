This is the readme.txt file for the gtmposix plugin.

-------------------------------------------------------------
OVERVIEW
-------------------------------------------------------------

gtmposix is a simple plugin to allow FIS GT.M (http://fis-gtm.com) application code to use selected POSIX functions on POSIX (UNIX/Linux) editions of GT.M. gtmposix provides a set of low-level calls wrapping and closely matching their corresponding POSIX functions, and a set of high-level entryrefs that provide a further layer of wrapping to make the functionality available in a form more familiar to M programmers.

gtmposix is just a wrapper for POSIX functions; it does not actually implement the underlying functionality.

gtmposix consists of the following files:

  - COPYING - the free / open source software (FOSS) license under which gtmposix is provided to you. See section LICENSE, below for more information, especially if COPYING is missing when you receive gtmposix.

  - gtmposix.c - C code that wraps POSIX functions for use by GT.M.

  - gtmposix.xc_proto - a prototype to generate the call-out table used by GT.M to map M entryrefs to C entry points, as described in the Programmers Guide, UNIX edition.

  - Makefile - for use by GNU make to build, test, install and uninstall the package.

  - _POSIX.m - wraps the C code with M-like functionality to provide ^%POSIX entryrefs.

  - posixtest.m - a simple test to check for correct installation and operation of gtmposix.

  - readme.txt - this file.

References to the Programmers Guide are to the UNIX edition.


-------------------------------------------------------------
LICENSE
-------------------------------------------------------------

If you receive this plugin integrated with a GT.M distribution, the license for your GT.M distribution is also your license for this plugin.

In the event the package contains a COPYING file, that is your license for this plugin. Except as noted here, you must retain that file and include it if you redistribute the package or a derivative work thereof.

If you have a signed agreement providing you with GT.M under a different license from that in the COPYING file, you may, at your option, delete the COPYING file and use gtmposix as an integral part of GT.M under the same terms as that of the GT.M license. If your GT.M license permits redistribution, you may redistribute gtmposix integrated with GT.M under the terms of that license.

Simple aggregation or bundling of this package with another for distribution does not create a derivative work. To make clear the distinction between this package and another with which it is aggregated or bundled, it suffices to place the package in a separate directory or folder when distributing the aggregate or bundle.

Should you receive this package not integrated with a GT.M distribution, and missing a COPYING file, you may create a file called COPYING from the GNU Affero General Public License Version 3 or later (https://www.gnu.org/licenses/agpl.txt) and use this package under the terms of that license.


-------------------------------------------------------------
INSTALLATION
-------------------------------------------------------------

gtmposix comes with a Makefile that you can use with GNU make to build, test, install and uninstall the package. Depending on the platform, GNU make may be available via "gmake" or "make" command. Building, testing, and using gtmposix does not require root access.  Installing the plugin in the $gtm_dist/plugin subdirectory requires root access. The targets in the Makefile designated for external use are:

  - all: creates libgtmposix.so (the shared library of C code that wraps POSIX functions) and gtmposix.xc (although this is a text file, the first line points to libgtmposix.so and gtmposix.xc must therefore be created by the Makefile

  - clean: delete object files and gtmposix.xc

  - install: executed as root to install gtmposix in $gtm_dist/plugin

  - test: after building gtmposix and before installation, a quick test for correct operation of the plugin

  - uninstall: executed as root to remove an installed plugin from under a GT.M installation

The following targets also exist, but are intended for use within the Makefile rather than for external invocation: gtmposix.o, gtmposix.xc, and libgtmposix.so.

Make always needs the following environment variable to be set: gtm_dist, the directory where GT.M is installed. If you plan to install the plugin for multiple GT.M versions, please use "make clean" before repeating the build, because the build includes gtmxc_types.h from $gtm_dist.

Depending on your GT.M installation, some make targets may need additional environment variables to be set:

  - make test sends a LOG_WARNING severity message and a LOG_INFO severity message and reads the syslog file for each to verify the messages. Although posixtest.m tries to make reasonable guesses about the location of the files on your system, it has no way to know how you have syslog configured. If you see a "FAIL syslog ..." output message repeat the test with the environment variable syslog_warning set to the location of the syslog file for LOG_WARNING messages. If you see a "FAIL SYSLOG ..." output message, repeat the test with the environment variable syslog_info set to the location of the syslog file for LOG_INFO messages. In particular, a test on Red Hat Enterprise Linux may require $syslog_info to be "/var/log/messages".

  - if your GT.M installation includes UTF-8 support (i.e., if it has a utf8 sub-directory), make install requires the environment variable LC_CTYPE to specify a valid UTF-8 locale, and depending on how libicu is built on your system, may require the gtm_icu_version to have the ICU version number. Refer to sub-section on ICU of Chapter 2, GT.M Language Extensions, for more information about gtm_icu_version and LC_CTYPE.


-------------------------------------------------------------
TESTING
-------------------------------------------------------------

The expected output of make test is as below; manually verify whether the statement about Daylight Savings Time is correct.

    PASS Invocation
    PASS $zhorolog
    PASS $ZHOROLOG
    Daylight Savings Time is in effect
    PASS mktime()
    PASS Microsecond resolution
    PASS regmatch^%POSIX 1
    PASS regfree^%POSIX
    PASS REGMATCH^%POSIX 1
    PASS REGFREE^%POSIX
    PASS regmatch^%POSIX 2
    PASS REGMATCH^%POSIX 2
    PASS regmatch^%POSIX 3
    PASS REGMATCH^%POSIX 3
    PASS regmatch^%POSIX 3
    PASS REGMATCH^%POSIX 3
    PASS regmatch^%POSIX 4
    PASS REGMATCH^%POSIX 4
    PASS regmatch^%POSIX 5
    PASS REGMATCH^%POSIX 5
    PASS mktmpdir
    PASS statfile.times
    PASS statfile.ids
    PASS filemodeconst^%POSIX
    PASS signal
    PASS STATFILE.times
    PASS STATFILE.ids
    PASS syslog
    PASS SYSLOG
    PASS setenv
    PASS unsetenv
    PASS rmdir
    PASS MKTMPDIR
    PASS mkdir
    PASS MKDIR
    PASS UTIMES
    PASS UMASK
    PASS CHMOD
    PASS SYMLINK
    PASS REALPATH
    PASS CP
    PASS Nanosecond resolution
    PASS SYSCONF


-------------------------------------------------------------
USE
-------------------------------------------------------------

For use by GT.M, the environment variable GTMXC_gtmposix must point to gtmposix.xc ($gtm_dist/plugin/gtmposix.xc after make install), the location of the gtmposix.xc file; and the environment variable gtmroutines must allow GT.M processes to find the %POSIX entryrefs. Depending on your platform, this includes a $gtmroutines term of the form $gtm_dist/plugin/o/_POSIX.so or $gtm_dist/plugin/o($gtm_dist/plugin/r) for M mode processes and $gtm_dist/plugin/o/utf8/_POSIX.so or $gtm_dist/plugin/o/utf8($gtm_dist/plugin/r) for UTF-8 mode processes.

For GT.M versions V5.5-000 and newer, the $gtm_dist/gtmprofile file that you can source to set environment variables and the $gtm_dist/gtm script to run GT.M automatically define appropriate values for $GTMXC_gtmposix and $gtmroutines to allow processes to execute gtmposix.

Note: you may need additional environment variables to install and use gtmposix, for example to preload the correct libraries if they are not automatically loaded. Contact your GT.M support channel for assistance with these environment variables.


(High level) ^%POSIX entryrefs
-------------------------------------------------------------

Except for any entryrefs starting with $$, which must be called as functions, ^%POSIX entryrefs as described below can be called either as functions or with a DO. Except where noted, each entry ref can be invoked in either all upper-case or all lower-case, but not with mixed case. These entryrefs have no abbreviations.

  - chmod^%POSIX(name,mode)
    Changes the permissions of a file to those specified, whether in symbolic or numeric representation.

  - clockgettime^%POSIX(clock,.sec,.nsec)
    Retrieves the time of the specified clock, in symbolic or numeric representation, with nanoosecond resolution. Note that nanosecond resolution does not mean nanosecond accuracy.

  - $$clockval^%POSIX(clockval)
    Given a symbolic clock ID as a string,, e.g., "CLOCK_REALTIME", returns the numeric value of that clock. See also the description of $&gtmposix.clockval().

  - cp^%POSIX(source,dest)
    Copy a file, preserving its permissions.

  - $$filemodeconst^%POSIX(sym)
    Given a symbolic file mode as a string,, e.g., "S_IRWXU", returns the numeric value of that mode. See also the description of $&gtmposix.filemodeconst().

  - mkdir^%POSIX(dirname,mode)
    Given a directory name as a string, and a mode, as either a symbolic or numeric value, creates the directory.

  - mktime^%POSIX(year,mon,mday,hour,min,sec,.wday,.yday,.isdst,.unixtime)
    Converts a broken-down time structure to calendar time representation, populating variables to contain the day of the week, day of the year, daylight saving status, and UNIX time.

  - mktmpdir^%POSIX(.template)
    With a directory name template ending in "XXXXXX" creates a directory with a unique name, replacing the "XXXXXX" to return the name of the directory created in template. On platforms where mkdtemp() is not available (AIX, HP-UX, and Solaris), GT.M uses mkdir to create a temporary directory with a random name created by GT.M.

  - realpath^%POSIX(name,.realpath)
    Retrieves the canonicalized absolute pathname to the file specified by name and stores it in realpath.

  - regfree^%POSIX(pregstrname)
    Given the *name* of a variable with a compiled regular expression as a string, frees the memory and ZKILLs the variable. Note that regfree() requires a *variable name* to be passed in as a string. For example, after regmatch^%POSIX("AIXHP-UXLinuxSolaris","ux","REG_ICASE",,.matches,1), the call to regfree to release the memory would be regfree^%POSIX("%POSIX(""regmatch"",""ux"",%POSIX(""regmatch"",""REG_ICASE""))")

  - regmatch^%POSIX(str,patt,pattflags,matchflags,.matchresults,maxresults)
    Regular expression matching in string str for pattern patt compiling the pattern if needed using regcomp() and matching using regmatch(). pattflags condition the pattern compilation with regcomp(). matchflags condition the matching performed by regexec(). To pass multiple flags, simply add the numeric values of the individual flags as provided by $$regsymval^%POSIX(). maxresults specifies the maximum number of matches. The function returns results as an array, where the value of matchresults(n,"start") provides the starting character position for the nth match, and the value of matchresults(n,"end") provides the character position for the first character after a match; e.g. $extract(str,matchresults(2,"start"),matchresults(2,"end")-1) returns the second matching substring. When called as a function, regmatch^%POSIX returns 1 on successful match and 0 if there was no match. On a successful match, the function KILLs all prior data in matchresults and otherwise leaves it unchanged. After a failed compilation, %POSIX("regcomp","errno") contains the error code from errlog(). When the match encounters an error (as opposed to a failure to match), %POSIX("regexec","errno") contains the value of errno. Local variable nodes %POSIX("regmatch",patt,pattflags) contain descriptors of compiled patterns and *must* *not* *be* *modified* *by* *your* *application* *code*. Be sure to read Memory Usage Considerations, below. Refer to man regex for more information about regular expressions and pattern matching.

  - $$regsymval^%POSIX(sym)
    Returns the numeric value of a symbolic constant used in regular expression pattern matching, such as "REG_ICASE". Also, it provides the sizes of certain structures that M code needs to have access to, when provided as strings, such as "sizeof(regex_t)", "sizeof(regmatch_t)", and "sizeof(regoff_t)".

  - rmdir^%POSIX(dirname)
    Removes a directory. For the call to succeed, the directory must be empty.

  - setenv^%POSIX(name,value,overwrite)
    Sets an environment variable to the specified value, overwriting or preserving the existing value as indicated.

  - statfile^%POSIX(f,.s)
    Provides information about file f in nodes of local variable s. All prior nodes of s are deleted. When called as a function, statfile returns 1 unless the underlying call to stat() failed. Refer to man 2 stat for more information.

  - symlink^%POSIX(target,name)
    Creates a symbolic link to a file with the specified name.

  - sysconf^%POSIX(name,.value)
    Obtains the value of the specified configuration option and saves it into the provided container.

  - $$sysconfval^%POSIX(option)
    Given a symbolic configuration option as a string,, e.g., "ARG_MAX", returns the numeric value of that option. See also the description of $&gtmposix.sysconfval().

  - syslog^%POSIX(message,format,facility,level)
    Provides a mechanism to log messages to the system log. format defaults to "%s", facility to "LOG_USER" and level to "LOG_INFO". When called as a function, syslog returns 1. Refer to man syslog for more information.

  - syslogval^%POSIX(msg)
    Given a symbolic syslog priority as a string,, e.g., "LOG_ALERT", returns the numeric value of that priority. See also the description of $&gtmposix.syslogval().

  - unsetenv^%POSIX(name)
    Unsets an environment variable.

  - umask^%POSIX(mode,.oldMode)
    Sets the current user's file mode creation mask, passed in as a symbolic or numeric value, and returns the previous mask's numeric value in the second argument.

  - utimes^%POSIX(name)
    Updates the access and modification timestamps of a file. The implemented functionality is equivalent to a "touch" command.

  - $$version^%POSIX
    Returns the version of the gtmposix plugin.

  - $$zhorolog^%POSIX
    Provides the time in $horolog format, but with microsecond resolution of the number of seconds since midnight. Note that microsecond resolution does not mean microsecond accuracy.


Examples of ^%POSIX usage
-------------------------------------------------------------

Below are examples of usage of high level entryrefs in ^%POSIX. The file posixtest.m contains examples of use of the functions in gtmposix.

    GTM>set str="THE QUICK BROWN FOX JUMPS OVER the lazy dog"

    GTM>write:$$regmatch^%POSIX(str,"the",,,.result) $extract(str,result(1,"start"),result(1,"end")-1)
    the
    GTM>write:$$regmatch^%POSIX(str,"the","REG_ICASE",,.result) $extract(str,result(1,"start"),result(1,"end")-1)
    THE
    GTM>

    GTM>set retval=$$statfile^%POSIX($ztrnlnm("gtm_dist")_"/mumps",.stat) zwrite stat
    stat("atime")=1332555721
    stat("blksize")=4096
    stat("blocks")=24
    stat("ctime")=1326986163
    stat("dev")=2052
    stat("gid")=0
    stat("ino")=6567598
    stat("mode")=33133
    stat("mtime")=1326986160
    stat("nlink")=1
    stat("rdev")=0
    stat("size")=8700
    stat("uid")=0

    GTM>write stat("mode")\$$filemodeconst^%POSIX("S_IFREG")#2 ; It is a regular file
    1
    GTM>

    GTM>do syslog^%POSIX(str) zsystem "tail -1 /var/log/messages"
    Mar 24 19:23:12 bhaskark mumps: THE QUICK BROWN FOX JUMPS OVER the lazy dog

    GTM>

    GTM>write $$version^%POSIX
    r1
    GTM>

    GTM>write $horolog," : ",$$zhorolog^%POSIX
    62626,60532 : 62626,60532.466276
    GTM>


(Low Level) gtmposix calls
-------------------------------------------------------------

The high level entryrefs in ^%POSIX access low level functions in gtmposix.c that directly wrap POSIX functions. Unless otherwise noted, functions return 0 for a successful completion, and non-zero otherwise. Note that some POSIX functions only return success, and also that a non-zero return value triggers a "%GTM-E-ZCSTATUSRET, External call returned error status" GT.M runtime error for your $ETRAP or $ZTRAP error handler. Where errno is the last argument passed by reference, it takes on the value of the errno from the underlying system call.

Note: The gtmposix GT.M interface to call out to POSIX functions is a low-level interface designed for use by programmers rather than end-users. Misuse, abuse and bugs can result in programs that are fragile, hard to troubleshoot and potentially insecure.

  - $&gtmposix.chmod(file,mode,.errno)
    Changes the permissions of a file to those specified. See man 2 chmod for more infornmation.

  - $&gtmposix.clockgettime(clock,.tvsec,.tvnsec,.errno)
    Returns the time of the specified clock in seconds and nanoseconds. See man clock_gettime on your POSIX system for more information.

  - $&gtmposix.clockval(fmsymconst,.symval)
    Takes a symbolic clock ID constant in fmsymconst and returns the numeric value in symval. If no such constant exists, the return value is non-zero. Currently supported fmsymconst constants are the following. Please see clock_gettime() function man page for their meaning.

	"CLOCK_HIGHRES",            "CLOCK_MONOTONIC", "CLOCK_MONOTONIC_RAW",
	"CLOCK_PROCESS_CPUTIME_ID", "CLOCK_REALTIME",  "CLOCK_THREAD_CPUTIME_ID"

  - $&gtmposix.cp(source,dest,.errno)
    Copy file source to dest, preserving its permissions. Note that this function is not a wrapper to a single POSIX function but a basic POSIX-conformant implementation of the cp command available on most UNIX OSs.

  - $&gtmposix.filemodeconst(fmsymconst,.symval)
    Takes a symbolic regular file mode constant in fmsymconst and returns the numeric value in symval. If no such constant exists, the return value is non-zero. Currently supported fmsymconst constants are the following. Please see stat() function man page for their meaning.

        "S_IFBLK",  "S_IFCHR", "S_IFDIR", "S_IFIFO", "S_IFLNK", "S_IFMT",  "S_IFREG",
        "S_IFSOCK", "S_IRGRP", "S_IROTH", "S_IRUSR", "S_IRWXG", "S_IRWXO", "S_IRWXU",
	"S_ISGID",  "S_ISUID", "S_ISVTX", "S_IWGRP", "S_IWOTH", "S_IWUSR", "S_IXGRP",
	"S_IXOTH",  "S_IXUSR"

  - $&gtmposix.gettimeofday(.tvsec,.tvusec,.errno)
    Returns the current time as the number of seconds since the UNIX epoch (00:00:00 UTC on 1 January 1970) and the number of microseconds within the current second. See man gettimeofday on your POSIX system for more information.

  - $&gtmposix.localtime(tvsec,.sec,.min,.hour,.mday,.mon,.year,.wday,.yday,.isdst,.errno)
    Takes a time value in tvsec represented as a number of seconds from the epoch - for example as returned by gettimeofday() - and returns a number of usable fields for that time value. See man localtime for more information.

  - $&gtmposix.mkdir(.dirname,mode,.errno)
    Creates a directory dirname with the specified permissions. See man 2 mkdir for more information.

  - $&gtmposix.mkdtemp(template,.errno)
    With a template for a temporary directory name - the last six characters must be "XXXXXX" - creates a unique temporary directory and updates template with the name. See man mkdtemp for more information.

  - $&gtmposix.mktime(year,month,mday,hour,min,sec,.wday,.yday,.isdst,.unixtime,.errno)
    Takes elements of POSIX broken-down time and returns time since the UNIX epoch in seconds in unixtime. Note that year is the offset from 1900 (i.e, 2014 is 114) and month is the offset from January (i.e., December is 11). wday is the day of the week offset from Sunday and yday is the day of the year offset from January 1 (note that the offsets of dates starting with March 1 vary between leap years and non-leap years). isdst should be initialized to one of 0, 1, or -1 as required by the POSIX mktime() function. If a $horolog value is the source of broken-down time, isdst should be -1 since GT.M $horolog reflects the state of Daylight Savings time in the timezone of the process, but the M application code does not know whether or not Daylight Savings Time is in effect; on return from the call, it is 0 if Daylight Savings Time is in effect and 1 if it is not. See man mktime for more information.

  - $&gtmposix.realpath(file,.result,.errno)
    Retrieves the canonicalized absolute pathname to the specified file and stores it in result. See man realpath for more information.

  - $&gtmposix.regcomp(.pregstr,regex,cflags,.errno)
    Takes a regular expression regex, compiles it and returns a pointer to a descriptor of the compiled regular expression in pregstr. Application code *must* *not* modify the value of pregstr. cflags specifies the type of regular expression compilation. See man regex for more information.

  - $&gtmposix.regconst(regsymconst,.symval)
    Takes a symbolic regular expression constant in regsymconst and returns the numeric value in symval. If no such constant exists, the return value is non-zero. The $$regsymval^%POSIX() function uses $&gtmposix.regconst(). Currently supported values of regsymconst are

	"REG_BADBR",      "REG_BADPAT",      "REG_BADRPT",         "REG_EBRACE",       "REG_EBRACK",    "REG_ECOLLATE",
	"REG_ECTYPE",     "REG_EESCAPE",     "REG_EPAREN",         "REG_ERANGE",       "REG_ESPACE",    "REG_ESUBREG",
	"REG_EXTENDED",   "REG_ICASE",       "REG_NEWLINE",        "REG_NOMATCH",      "REG_NOSUB",     "REG_NOTBOL",
	"REG_NOTEOL",     "sizeof(regex_t)", "sizeof(regmatch_t)", "sizeof(regoff_t)"

  - $&gtmposix.regexec(pregstr,string,nmatch,.pmatch,eflags,.matchsuccess)
    Takes a string in string and matches it against a previously compiled regular expression whose descriptor is in pregstr with matching flags in eflags, for which numeric values can be obtained from symbolic values with $$regconst^%POSIX(). nmatch is the maximum number of matches to be returned and pmatch is a predefined string in which the function returns information about substrings matched. pmatch must be initialized to at least nmatch times the size of each match result which you can effect with: set $zpiece(pmatch,$zchar(0),nmatch*$$regsymval("sizeof(regmatch_t)")+1)="" matchsuccess is 1 if the match was successful, 0 if not. The return value is 0 for both successful and failing matches; a non-zero value indicates an error. See man regex for more information.

  - $&gtmposix.regfree(pregstr)
    Takes a descriptor for a compiled regular expression, as provided by $&gtmposix.regcomp() and frees the memory associated with the compiled regular expression. After executing $&gtmposix.regfree(), the descriptor can be safely deleted; deleting a descriptor prior to calling this function results in a memory leak because deleting the descriptor makes the memory used for the compiled expression unrecoverable.

  - $&gtmposix.regofft2int(regofftbytes,.regofftint)
    On both little- and big-endian platforms, takes a sequence of bytes of size sizeof(regoff_t) and returns it as an integer. $$regsconst^%POSIX("sizeof(regoff_t)") provides the size of regoff_t. Always returns 0.

  - $&gtmposix.rmdir(pathname,.errno)
    Removes a directory, which must be empty. See man 2 rmdir for more information.

  - $&gtmposix.setenv(name,value,overwrite,.errno)
    Sets the value of an environment variable. name is the name of an environment variable (i.e., without a leading "$") and value is the value it is to have ($char(0) cannot be part of the value). If the name already has a value, then overwrite must be non-zero in order to replace the existing value. See man setenv for more information.

  - $&gtmposix.signalval(signame,.sigval)
    Takes a signal name (such as "SIGUSR1") and provides its value in sigval. A non-zero return value means that no value was found for the name. Currently supported signames are

	"SIGABRT", "SIGALRM", "SIGBUS",  "SIGCHLD", "SIGCONT", "SIGFPE",  "SIGHUP",  "SIGILL",
	"SIGINT",  "SIGKILL", "SIGPIPE", "SIGQUIT", "SIGSEGV", "SIGSTOP", "SIGTERM", "SIGTRAP",
	"SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG",  "SIGUSR1", "SIGUSR2", "SIGXCPU", "SIGXFSZ"

  - $&gtmposix.stat(fname,.dev,.ino,.mode,.nlink,.uid,.gid,.rdev,.size,.blksize,.blocks,.atime,.atimen,.mtime,mtimen,.ctime,.ctimen,.errno)
    Takes the name of a file in fname, and provides information about it. See man 2 stat for more information.

  - $&gtmposix.symlink(target,name,.errno)
    Creates a symbolic link to a file with the specified name. See man symlink for more information.

  - $&gtmposix.sysconf(name,.value,.errno)
    Obtains the value of the specified configuration option and saves it to value. The name argument needs to be a valid int understandable by sysconf() rather than a corresponding system-defined constant. For instance, _SC_ARG_MAX and _SC_2_VERSION's values should be used for ARG_MAX and POSIX2_VERSION options, respectively. Note that for certain limits the value of -1 can be legitimately returned, indicating that there is no definite limit. See man sysconf for more information.

  - $&gtmposix.sysconfval(fmsymconst,.symval)
    Takes a sysconf option name (such as "PAGESIZE") and provides the corresponding _SC... value in sigval. A non-zero return value means that no value was found for the name. Currently supported sysconf options are

        "ARG_MAX",          "BC_BASE_MAX",   "BC_DIM_MAX",      "BC_SCALE_MAX",    "BC_STRING_MAX",   "CHILD_MAX",
       	"COLL_WEIGHTS_MAX", "EXPR_NEST_MAX", "HOST_NAME_MAX",   "LINE_MAX",        "LOGIN_NAME_MAX",  "OPEN_MAX",
       	"PAGESIZE",         "POSIX2_C_DEV",  "POSIX2_FORT_DEV", "POSIX2_FORT_RUN", "POSIX2_SW_DEV",   "POSIX2_VERSION",
       	"RE_DUP_MAX",       "STREAM_MAX",    "SYMLOOP_MAX",     "TTY_NAME_MAX",    "TZNAME_MAX",      "_POSIX2_LOCALEDEF",
       	"_POSIX_VERSION"

  - $&gtmposix.syslog(priority,message,.errno)
    Takes a priority, format and message to log on the system log. Priority is itself an OR of a facility and a level. See man syslog for more information.

  - $&gtmposix.syslogconst(syslogsymconst,.syslogsymval)
    Takes a symbolic syslog facility or level name (e.g., "LOG_USER") in syslogsymconst and returns its value in syslogsymval. A non-zero return value means that a value was not found. Currently supported values of syslogsymconst are

        "LOG_ALERT",  "LOG_CRIT",   "LOG_DEBUG",  "LOG_EMERG",  "LOG_ERR",    "LOG_INFO",   "LOG_LOCAL0",
	"LOG_LOCAL1", "LOG_LOCAL2", "LOG_LOCAL3", "LOG_LOCAL4", "LOG_LOCAL5", "LOG_LOCAL6", "LOG_LOCAL7",
	"LOG_NOTICE", "LOG_USER",   "LOG_WARNING"

  - $&gtmposix.umask(mode,.prevMode,.errno)
    Sets the current user's file mode creation mask and returns the previous mask in the second argument. See man umask for more information.

  - $&gtmposix.unsetenv(name,.errno)
    Unsets the value of an environment variable. See man umask for more information.

  - $&gtmposix.utimes(file,.errno)
    Updates the access and modification timestamps of a file. See man utimes for more information.

posixtest.m contains examples of use of the low level gtmposix interfaces.


The %POSIX local variable
-------------------------------------------------------------

The gtmposix plugin uses the %POSIX local variable to store information pertaining to POSIX external calls. For example, a call to $&regsymval^%POSIX("REG_NOTBOL") that returns a numeric value also sets the node %POSIX("regmatch","REG_NOTBOL") to that value. Subsequent calls to $$regsymval^%POSIX("REG_NOTBOL") return the value stored in %POSIX rather than calling out the low level function. This means that KILLs or NEWs that remove the value in %POSIX, result in a call to the low level function, and SETs of values may cause inappropriate results from subsequent invocations.

If your application already uses %POSIX for another purpose, you can edit _POSIX.m and replace all occurrences of %POSIX with another available local variable name.


Memory Usage Considerations
-------------------------------------------------------------

When $&gtmposix.regcomp() is called to compile a regular expression, it allocates needed memory, and returns a descriptor to the compiled code. Until a subsequent call to $&gtmposix.regfree() with that descriptor, the memory is retained. The high level regmatch^%POSIX() entryref stores descriptors in %POSIX("regmatch",...) nodes. If an application deletes or modifies these nodes prior to calling $&gtmposix.regfree() to release compiled regular expressions, that memory cannot be released during the life of the process. If your application uses scope management (using KILL and/or NEW) that adversely interacts with this, you should consider modifying _POSIX.m to free the cached compiled regular expression immediately after the call to $&gtmposix.regexec(), or to store the descriptors in a global variable specific to the process, rather than in a local variable.


Error Handling
-------------------------------------------------------------

Entryrefs within ^%POSIX except the top one (calling which is not meaningful), raise errors but do not set their own error handlers with $ETRAP or $ZTRAP. Application code error handlers should deal with these errors. In particular, note that non-zero function return values from $&gtmposix functions result in ZCSTATUSRET errors.

Look at the end of _POSIX.m for errors raised by entryrefs in %POSIX.


-------------------------------------------------------------
UPDATES
-------------------------------------------------------------

    POSIX r2:

      - Installation of the GT.M POSIX plugin succeeds in the case where the standard utility programs are in $gtm_dist/libgtmutil.so but not in $gtm_dist/*.o files, and other GT.M programs have been added post-installation to the $gtm_dist/directory (GTMDefinedTypesInit being a representative example). Previously, under such conditions, make of the POSIX plugin would fail with a GTM-E-OBJFILERR for _DATE.o. (GTM-8307)

      - $&gtmposix.cp() correctly copies a file. Previously, if the copy destination targeted an existing file that exceeded the source file size, the copy operation wrote the contents of the source file leaving all preexisting data beyond the size of the source unchanged. (GTM-8267)

    POSIX r1:

      - Added bindings for the following POSIX functions: chmod(), clock_gettime(), realpath(), symlink(), sysconf(), umask(), and utimes(). Additionally, implemented cp(), a function for copying files, as an in-process alternative to the UNIX cp command. See above for more details. (GTM-8250)

      - $&gtmposix.stat() provides three additional parameters to return the nanosecond values of file creation, access, and modification timestamps respectively. Previously, the timestamp values were only available in seconds. Note that this change is not backward compatible because the number of parameters has changed. (GTM-8252)

      - Invoking low-level gtmposix functions with fewer arguments than parameters correctly returns a non-zero value, thus triggering a ZCSTATUSRET error. Previously, invoking such functions (for example, $%gtmposix.stat()) with insufficient number of arguments could result in a segmentation violation (SIG-11). (GTM-8255)

      - The gtmposix plug-in abolishes the $horolog versioning format in favor of single incremental integers prefixed by "r" and signifying the current revision number, such as "r1". (GTM-8257)

    POSIX 63250_37800:

      - Added binding for POSIX function mktime(); please see above for more details. Improved error checking for all implemented functions as follows:

        * posix_gettimeofday returns errno as appropriate. Previously it returned success (0) or failure (-1), but no detailed failure information.

        * posix_localtime returns errno as appropriate for those platforms that support it (currently AIX and HP-UX). Previously it returned success (0) or failure (-1), as it still does for Linux and Solaris, but no detailed failure information.

        * In case of an argument mismatch, posix_mkdir, posix_mkdtemp, posix_regcomp, and posix_stat each set errno as the negative of the input argument count.

        * posix_regcomp and posix_regexec each set errno to the return code from the POSIX function regcomp() and regex(). Previously these functions returned errno even though these functions do not set it.

        * posix_regexec leaves the matched result data buffer unmodified. Previously it copied data from an uninitialized pointer.

        All functions now enforce variable type conversions. (GTM-7969)

    POSIX 63202_39000:

      - The stat() function available through the gtmposix plug-in no longer errors out due to the incorrect expected argument count estimation. Previous versions of gtmposix contained an incorrect stat() function definition in the external call table, which, if used with GT.M V6.0-003, results in error returns even on correct external routine invocations. (GTM-7788)

      - "make test" does not issue DLLNOOPEN error due to unknown directory of the ICU library. Previously, "make test" was halting with DLLNOOPEN under UTF-8 mode. (GTM-7875)

      - $$filemodeconst^%POSIX() can also report the values for S_IFMT, S_IFSOCK, S_IFLNK, S_IFREG, S_IFBLK, S_IFDIR, S_IFCHR, and S_IFIFO. (GTM-7930)

    POSIX 62925_54000:

      - $$ZHOROLOG^%POSIX returns correct number of seconds and microseconds. Previously, when the number of microseconds was 0, the function reported ten times the seconds count with no fractional part. (GTM-7725)
