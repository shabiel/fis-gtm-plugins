/****************************************************************
 *								*
 * Copyright (c) 2012-2015 Fidelity National Information 	*
 * Services, Inc. and/or its subsidiaries. All rights reserved.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/
/* Caution - these functions are not thread-safe */

#include <errno.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "gtmxc_types.h"

#define MAXREGEXMEM	65536	/* Maximum memory to allocate for a compiled regular expression */
#define MINMALLOC   	64	/* Minimum space to request from gtm_malloc - MAXREGEXMEM/MINMALLOC should be a power of 4 */
#define CP_BUF_SIZE	4096	/* Size of the buffer used for copying a file. */

/* On certain platforms the st_Xtime field of the stat structure got replaced by a timespec st_Xtim field, which in turn has tv_sec
 * and tv_nsec fields. For compatibility reasons, those platforms define an st_Xtime macro which points to st_Xtim.tv_sec. Whenever
 * we detect such a situation, we define a nanosecond flavor of that macro to point to st_Xtim.tv_nsec. On HPUX Itanium and older
 * AIX boxes the stat structure simply has additional fields with the nanoseconds value, yet the names of those field are different
 * on those two architectures, so we choose our mapping accordingly.
 */
#if defined st_atime
#  define st_natime	st_atim.tv_nsec
#elif defined(_AIX)
#  define st_natime	st_atime_n
#elif defined(__hpux) && defined(__ia64)
#  define st_natime	st_natime
#endif
#if defined st_ctime
#  define st_nctime	st_ctim.tv_nsec
#elif defined(_AIX)
#  define st_nctime	st_ctime_n
#elif defined(__hpux) && defined(__ia64)
#  define st_nctime	st_nctime
#endif
#if defined st_mtime
#  define st_nmtime	st_mtim.tv_nsec
#elif defined(_AIX)
#  define st_nmtime	st_mtime_n
#elif defined(__hpux) && defined(__ia64)
#  define st_nmtime	st_nmtime
#endif

/* Translation tables for various include file #define names to the platform values for those names */
/* Names *must* be in alphabetic order of strings; otherwise search will return incorrect results */

#define EINTR_OPER(ACTION, RC)				\
{							\
	do						\
	{						\
		RC = ACTION;				\
	} while ((-1 == RC) && (EINTR == errno));	\
}

static const char *clocks[] =
{
#	ifdef CLOCK_HIGHRES
	"CLOCK_HIGHRES",
#	endif
#	ifdef CLOCK_MONOTONIC
	"CLOCK_MONOTONIC",
#	endif
#	ifdef CLOCK_MONOTONIC_RAW
	"CLOCK_MONOTONIC_RAW",
#	endif
#	ifdef CLOCK_PROCESS_CPUTIME_ID
	"CLOCK_PROCESS_CPUTIME_ID",
#	endif
	"CLOCK_REALTIME"
#	ifdef CLOCK_THREAD_CPUTIME_ID
	,"CLOCK_THREAD_CPUTIME_ID"
#	endif
};
static const gtm_int_t clock_values[] =
{
#	ifdef CLOCK_HIGHRES
	CLOCK_HIGHRES,
#	endif
#	ifdef CLOCK_MONOTONIC
	CLOCK_MONOTONIC,
#	endif
#	ifdef CLOCK_MONOTONIC_RAW
	CLOCK_MONOTONIC_RAW,
#	endif
#	ifdef CLOCK_PROCESS_CPUTIME_ID
	CLOCK_PROCESS_CPUTIME_ID,
#	endif
	CLOCK_REALTIME
#	ifdef CLOCK_THREAD_CPUTIME_ID
	,CLOCK_THREAD_CPUTIME_ID,
#	endif
};

static const char *fmodes[] =
{
	"S_IFBLK",  "S_IFCHR", "S_IFDIR", "S_IFIFO", "S_IFLNK", "S_IFMT",  "S_IFREG",
	"S_IFSOCK", "S_IRGRP", "S_IROTH", "S_IRUSR", "S_IRWXG", "S_IRWXO", "S_IRWXU",
	"S_ISGID",  "S_ISUID", "S_ISVTX", "S_IWGRP", "S_IWOTH", "S_IWUSR", "S_IXGRP",
	"S_IXOTH",  "S_IXUSR"
};
static const gtm_int_t fmode_values[] =
{
	S_IFBLK,  S_IFCHR, S_IFDIR, S_IFIFO, S_IFLNK, S_IFMT,  S_IFREG,
	S_IFSOCK, S_IRGRP, S_IROTH, S_IRUSR, S_IRWXG, S_IRWXO, S_IRWXU,
	S_ISGID,  S_ISUID, S_ISVTX, S_IWGRP, S_IWOTH, S_IWUSR, S_IXGRP,
	S_IXOTH,  S_IXUSR
};

static const char *priority[] =
{
	"LOG_ALERT",  "LOG_CRIT",   "LOG_DEBUG",  "LOG_EMERG", "LOG_ERR",
	"LOG_INFO",   "LOG_LOCAL0", "LOG_LOCAL1", "LOG_LOCAL2",
	"LOG_LOCAL3", "LOG_LOCAL4", "LOG_LOCAL5", "LOG_LOCAL6",
	"LOG_LOCAL7", "LOG_NOTICE", "LOG_USER",   "LOG_WARNING"
};
static const gtm_int_t priority_values[] =
{
	LOG_ALERT,  LOG_CRIT,   LOG_DEBUG,  LOG_EMERG,  LOG_ERR,
	LOG_INFO,   LOG_LOCAL0, LOG_LOCAL1, LOG_LOCAL2,
	LOG_LOCAL3, LOG_LOCAL4, LOG_LOCAL5, LOG_LOCAL6,
	LOG_LOCAL7, LOG_NOTICE, LOG_USER,   LOG_WARNING
};

static const char *regxflags[] =
{
	"REG_BADBR",        "REG_BADPAT",         "REG_BADRPT",       "REG_EBRACE",      "REG_EBRACK",    "REG_ECOLLATE",
	"REG_ECTYPE",       "REG_EESCAPE",        "REG_EPAREN",       "REG_ERANGE",      "REG_ESPACE",    "REG_ESUBREG",
	"REG_EXTENDED",     "REG_ICASE",          "REG_NEWLINE",      "REG_NOMATCH",     "REG_NOSUB",
	"REG_NOTBOL",       "REG_NOTEOL",
	"sizeof(regex_t)",  "sizeof(regmatch_t)", "sizeof(regoff_t)"
};
static const gtm_int_t regxflag_values[] =
{
	REG_BADBR,         REG_BADPAT,         REG_BADRPT,       REG_EBRACE,     REG_EBRACK,    REG_ECOLLATE,
	REG_ECTYPE,        REG_EESCAPE,        REG_EPAREN,       REG_ERANGE,     REG_ESPACE,    REG_ESUBREG,
	REG_EXTENDED,      REG_ICASE,          REG_NEWLINE,      REG_NOMATCH,    REG_NOSUB,
	REG_NOTBOL,        REG_NOTEOL,
	sizeof(regex_t),   sizeof(regmatch_t), sizeof(regoff_t)
};

static const char *signals[] =
{
	"SIGABRT", "SIGALRM", "SIGBUS",  "SIGCHLD", "SIGCONT", "SIGFPE",  "SIGHUP", "SIGILL",
	"SIGINT",  "SIGKILL", "SIGPIPE", "SIGQUIT", "SIGSEGV", "SIGSTOP", "SIGTERM",
	"SIGTRAP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG",  "SIGUSR1", "SIGUSR2",
	"SIGXCPU", "SIGXFSZ"
};
static const gtm_int_t signal_values[] =
{
	SIGABRT, SIGALRM, SIGBUS,  SIGCHLD, SIGCONT, SIGFPE,  SIGHUP, SIGILL,
	SIGINT,  SIGKILL, SIGPIPE, SIGQUIT, SIGSEGV, SIGSTOP, SIGTERM,
	SIGTRAP, SIGTSTP, SIGTTIN, SIGTTOU, SIGURG,  SIGUSR1, SIGUSR2,
	SIGXCPU, SIGXFSZ
};

static const char *sysconfs[] =
{
	"ARG_MAX",          "BC_BASE_MAX",   "BC_DIM_MAX",      "BC_SCALE_MAX",    "BC_STRING_MAX",   "CHILD_MAX",
	"COLL_WEIGHTS_MAX", "EXPR_NEST_MAX", "HOST_NAME_MAX",   "LINE_MAX",        "LOGIN_NAME_MAX",  "OPEN_MAX",
	"PAGESIZE",         "POSIX2_C_DEV",  "POSIX2_FORT_DEV", "POSIX2_FORT_RUN", "POSIX2_SW_DEV",   "POSIX2_VERSION",
	"RE_DUP_MAX",       "STREAM_MAX",    "SYMLOOP_MAX",     "TTY_NAME_MAX",    "TZNAME_MAX",      "_POSIX2_LOCALEDEF",
	"_POSIX_VERSION"
};
static const gtm_int_t sysconf_values[] =
{
	_SC_ARG_MAX,          _SC_BC_BASE_MAX,   _SC_BC_DIM_MAX,    _SC_BC_SCALE_MAX, _SC_BC_STRING_MAX,  _SC_CHILD_MAX,
	_SC_COLL_WEIGHTS_MAX, _SC_EXPR_NEST_MAX, _SC_HOST_NAME_MAX, _SC_LINE_MAX,     _SC_LOGIN_NAME_MAX, _SC_OPEN_MAX,
	_SC_PAGESIZE,         _SC_2_C_DEV,       _SC_2_FORT_DEV,    _SC_2_FORT_RUN,   _SC_2_SW_DEV,       _SC_2_VERSION,
	_SC_RE_DUP_MAX,       _SC_STREAM_MAX,    _SC_SYMLOOP_MAX,   _SC_TTY_NAME_MAX, _SC_TZNAME_MAX,     _SC_2_LOCALEDEF,
	_SC_VERSION
};

/* POSIX routines */

gtm_status_t posix_chmod(int argc, gtm_char_t *file, gtm_int_t mode, gtm_int_t *err_num)
{
	if (3 != argc)
		return (gtm_status_t)-argc;
	*err_num = (-1 == chmod(file, (mode_t)mode)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_clock_gettime(int argc, gtm_int_t clk_id, gtm_long_t *tv_sec, gtm_long_t *tv_nsec, gtm_int_t *err_num)
{
	struct timespec tp;

	if (4 != argc)
		return (gtm_status_t)-argc;
	if (0 == clock_gettime((clockid_t)clk_id, &tp))
	{
		*tv_sec = (gtm_long_t)tp.tv_sec;
		*tv_nsec = (gtm_long_t)tp.tv_nsec;
		*err_num = 0;
	} else
		*err_num = (gtm_int_t)errno;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_cp(int argc, gtm_char_t *source, gtm_char_t *dest, gtm_int_t *err_num)
{
	int		fd1, fd2, rc;
	char		*buf_ptr;
	char		buffer[CP_BUF_SIZE];
	struct stat	statbuf;
	ssize_t		read_count, written_count;

	if (3 != argc)
		return (gtm_status_t)-argc;
	if (-1 == stat((char *)source, &statbuf))
	{
		*err_num = (gtm_int_t)errno;
		return (gtm_status_t)*err_num;
	}
	EINTR_OPER(open(source, O_RDONLY), fd1);
	if (-1 == fd1)
	{
		*err_num = (gtm_int_t)errno;
		return (gtm_status_t)*err_num;
	}
	EINTR_OPER(open(dest, O_WRONLY | O_CREAT, statbuf.st_mode), fd2);
	if (-1 == fd2)
	{
		*err_num = (gtm_int_t)errno;
		EINTR_OPER(close(fd1), rc);
		return (gtm_status_t)*err_num;
	}
	EINTR_OPER(ftruncate(fd2, 0), rc);
	if (-1 == rc)
	{
		*err_num = (gtm_int_t)errno;
		EINTR_OPER(close(fd1), rc);
		EINTR_OPER(close(fd2), rc);
		return (gtm_status_t)*err_num;
	}
	*err_num = 0;
	do
	{
		EINTR_OPER(read(fd1, buffer, CP_BUF_SIZE), read_count);
		if (0 < read_count)
		{
			written_count = 0;
			buf_ptr = buffer;
			while (0 < read_count)
			{
				EINTR_OPER(write(fd2, buf_ptr, read_count), written_count);
				if (0 < written_count)
				{
					read_count -= written_count;
					buf_ptr += written_count;
				} else
				{
					if (-1 == written_count)
						*err_num = (gtm_int_t)errno;
					break;
				}
			}
		} else
		{
			if (-1 == read_count)
				*err_num = (gtm_int_t)errno;
			break;
		}
	} while (0 == *err_num);
	EINTR_OPER(close(fd1), rc);
	EINTR_OPER(close(fd2), rc);
	if (0 == *err_num)
		*err_num = (-1 == chmod(dest, statbuf.st_mode)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_gettimeofday(int argc, gtm_long_t *tv_sec, gtm_long_t *tv_usec, gtm_int_t *err_num)
{
	struct timeval currtimeval;

	if (3 != argc)
		return (gtm_status_t)-argc;
	if (-1 == gettimeofday(&currtimeval, NULL))
		*err_num = (gtm_int_t)errno;
	else
	{
		*tv_sec = (gtm_long_t)currtimeval.tv_sec;
		*tv_usec = (gtm_long_t)currtimeval.tv_usec;
		*err_num = 0;
	}
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_localtime(int argc, gtm_long_t timep, gtm_int_t *sec, gtm_int_t *min, gtm_int_t *hour,
			     gtm_int_t *mday, gtm_int_t *mon, gtm_int_t *year, gtm_int_t *wday,
			     gtm_int_t *yday, gtm_int_t *isdst, gtm_int_t *err_num)
{
	struct tm *currtimetm;

	if (11 != argc)
		return (gtm_status_t)-argc;
	if (currtimetm = localtime((time_t *)&timep))	/* Warning - assignment */
	{
		*sec	= (gtm_int_t)currtimetm->tm_sec;
		*min	= (gtm_int_t)currtimetm->tm_min;
		*hour	= (gtm_int_t)currtimetm->tm_hour;
		*mday	= (gtm_int_t)currtimetm->tm_mday;
		*mon	= (gtm_int_t)currtimetm->tm_mon;
		*year	= (gtm_int_t)currtimetm->tm_year;
		*wday	= (gtm_int_t)currtimetm->tm_wday;
		*yday	= (gtm_int_t)currtimetm->tm_yday;
		*isdst	= (gtm_int_t)currtimetm->tm_isdst;
		*err_num = 0;
	} else
	{
#		if defined __SunOS || defined __linux__
		/* Linux & Solaris do not set errno as required by POSIX std as of "IEEE Std 1003.1-2001" */
		*err_num = (gtm_int_t)-1;
#		else
		*err_num = (gtm_int_t)errno;
#		endif
	}
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_mkdir(int argc, gtm_char_t *dirname, gtm_int_t mode, gtm_int_t *err_num)
{
	if (3 != argc)
		return (gtm_status_t)-argc;
	/* Possible return codes on error are EACCESS, EDQUOT, EEXIST, EFAULT, ELOOP, EMLINK, ENAMETOOLONG,
	 * ENOENT, ENOMEM, ENOSPC, ENOTDIR, EPERM, and EROFS.
	 */
	*err_num = (-1 == mkdir((char *)dirname, (mode_t)mode)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_mkdtemp(int argc, gtm_char_t *template, gtm_int_t *err_num)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
#	if defined __SunOS || defined __hpux || defined _AIX
	/* Return -1 with template unchanged since neither HP-UX nor Solaris support mkdtemp() and AIX only
	 * supports it effective version 7.x.
	 */
	*err_num = (gtm_int_t)-1;
#	else
	/* Possible return codes on error are EACCESS, EDQUOT, EEXIST, EFAULT, ELOOP, EMLINK, ENAMETOOLONG,
	 * ENOENT, ENOMEM, ENOSPC, ENOTDIR, EPERM, EROFS.
	 */
	*err_num = (mkdtemp((char *)template)) ? 0 : (gtm_int_t)errno;
#	endif
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_mktime(int argc, gtm_int_t year, gtm_int_t mon, gtm_int_t mday, gtm_int_t hour,
			  gtm_int_t min, gtm_int_t sec, gtm_int_t *wday, gtm_int_t *yday, gtm_int_t *isdst,
			  gtm_long_t *unixtime, gtm_int_t *err_num)
{
	struct tm time_str;

	if (11 != argc)
		return (gtm_status_t)-argc;
	time_str.tm_year	= (int)year;
	time_str.tm_mon		= (int)mon;
	time_str.tm_mday	= (int)mday;
	time_str.tm_hour	= (int)hour;
	time_str.tm_min		= (int)min;
	time_str.tm_sec		= (int)sec;
	time_str.tm_isdst	= (int)(*isdst);
	if (-1 == (*unixtime = (gtm_long_t)mktime(&time_str)))	/* Warning - assignment */
	{
#	if defined __SunOS || defined __hpux
		/* Solaris and HPUX set errno for mktime */
		*err_num = (gtm_int_t)errno;
#	else
		*err_num = (gtm_int_t)-1;
#	endif
	} else
	{
		*wday = (gtm_int_t)time_str.tm_wday;
		*yday = (gtm_int_t)time_str.tm_yday;
		/* Only set DST if passed -1 */
		if (-1 == *isdst)
			*isdst = (gtm_int_t)time_str.tm_isdst;
		*err_num = 0;
	}
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_realpath(int argc, gtm_char_t *file, gtm_string_t *result, gtm_int_t *err_num)
{
	if (3 != argc)
		return (gtm_status_t)-argc;
	if (realpath((const char *)file, result->address))
	{
		result->length = strlen(result->address);
		*err_num = 0;
	} else
	{
		result->length = 0;
		*err_num = (gtm_int_t)errno;
	}
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_regcomp(int argc, gtm_string_t *pregstr, gtm_char_t *regex, gtm_int_t cflags, gtm_int_t *err_num)
{
	regex_t *preg;

	if (4 != argc)
		return (gtm_status_t)-argc;
	preg = (regex_t *)gtm_malloc(sizeof(regex_t));
	/* While AIX _may_ set errno, the open group specifically calls this function out as NOT setting errno. Each
	 * platform uses a set of return codes which are all defined in /usr/include/regex.h. All but Solaris list
	 * these error codes in the man page.
	 */
	*err_num = (gtm_int_t)regcomp(preg, regex, (int)cflags);
	if (0 == *err_num)
	{
		(pregstr->length) = sizeof(gtm_char_t *);
		memcpy(pregstr->address, &preg, pregstr->length);
	}
	return (gtm_status_t)*err_num;
}

/* posix_regexec() does not entirely follow the implementation of the POSIX regexec(). The latter returns 0 for a
 * successful match, REG_NOMATCH otherwise. But returning non-zero to GT.M from a C function will invoke the GT.M
 * error trap, which is not desirable for the non-match of a pattern.  Therefore, posix_regexec() always returns
 * zero and the result of the match is in the parameter *matchsuccess with 1 meaning a successful match and 0
 * otherwise.
 */
gtm_status_t posix_regexec(int argc, gtm_string_t *pregstr, gtm_char_t *string, gtm_int_t nmatch, gtm_string_t *pmatch,
			   gtm_int_t eflags, gtm_int_t *matchsuccess)
{
        regex_t         *preg;
	regmatch_t	*result;
	size_t		resultsize;

	if (6 != argc)
		return (gtm_status_t)-argc;
	memcpy(&preg, pregstr->address, pregstr->length);
	resultsize = nmatch * sizeof(regmatch_t);
	result = (regmatch_t *)gtm_malloc(resultsize);
	*matchsuccess = (0 == regexec(preg, (char *)string, (size_t)nmatch, result, (int)eflags));
	if (*matchsuccess)
		memcpy(pmatch->address, result, resultsize);
	gtm_free((void*)result);
	return 0;
}

gtm_status_t posix_regfree(int argc, gtm_string_t *pregstr)
{
	regex_t	*preg;

	if (1 != argc)
		return (gtm_status_t)-argc;
	memcpy(&preg, pregstr->address, pregstr->length);
	/* regfree is a void function */
	regfree(preg);
	gtm_free((void *)preg);
	return 0;
}

gtm_status_t posix_rmdir(int argc, gtm_char_t *pathname, gtm_int_t *err_num)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	*err_num = (-1 == rmdir((const char *)pathname)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_setenv(int argc, gtm_char_t *name, gtm_char_t *value, gtm_int_t overwrite, gtm_int_t *err_num)
{
	if (4 != argc)
		return (gtm_status_t)-argc;
	*err_num = (-1 == setenv((char *)name, (char *)value, (int)overwrite)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_stat(int argc, gtm_char_t *fname, gtm_ulong_t *dev, gtm_ulong_t *ino, gtm_ulong_t *mode,
			gtm_ulong_t *nlink, gtm_ulong_t *uid, gtm_ulong_t *gid, gtm_ulong_t *rdev, gtm_long_t *size,
			gtm_long_t *blksize, gtm_long_t *blocks, gtm_long_t *atime, gtm_long_t *atimen, gtm_long_t *mtime,
			gtm_long_t *mtimen, gtm_long_t *ctime, gtm_long_t *ctimen, gtm_int_t *err_num)
{
	struct stat	thisfile;
	gtm_status_t	retval;

	if (18 != argc)
		return (gtm_status_t)-argc;
	*err_num = (-1 == stat((char *)fname, &thisfile)) ? (gtm_int_t)errno : 0;
	if (0 == *err_num)
	{
		*dev     = (gtm_ulong_t)thisfile.st_dev;	/* ID of device containing file */
		*ino     = (gtm_ulong_t)thisfile.st_ino;	/* inode number */
		*mode    = (gtm_ulong_t)thisfile.st_mode;	/* protection */
		*nlink   = (gtm_ulong_t)thisfile.st_nlink;	/* number of hard links */
		*uid     = (gtm_ulong_t)thisfile.st_uid;	/* user ID of owner */
		*gid     = (gtm_ulong_t)thisfile.st_gid;	/* group ID of owner */
		*rdev    = (gtm_ulong_t)thisfile.st_rdev;	/* device ID (if special file) */
		*size    = (gtm_long_t)thisfile.st_size;	/* total size, in bytes */
		*blksize = (gtm_long_t)thisfile.st_blksize;	/* blocksize for file system I/O */
		*blocks  = (gtm_long_t)thisfile.st_blocks;	/* number of 512B blocks allocated */
		*atime   = (gtm_long_t)thisfile.st_atime;	/* time (secs) of last access */
		*atimen  = (gtm_long_t)thisfile.st_natime;	/* time (nsecs) of last access */
		*mtime   = (gtm_long_t)thisfile.st_mtime;	/* time (secs) of last modification */
		*mtimen  = (gtm_long_t)thisfile.st_nmtime;	/* time (nsecs) of last modification */
		*ctime   = (gtm_long_t)thisfile.st_ctime;	/* time (secs) of last status change */
		*ctimen  = (gtm_long_t)thisfile.st_nctime;	/* time (nsecs) of last status change */
	}
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_symlink(int argc, gtm_char_t *target, gtm_char_t *name, gtm_int_t *err_num)
{
	if (3 != argc)
		return (gtm_status_t)-argc;
	*err_num = (-1 == symlink(target, name)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_sysconf(int argc, gtm_int_t name, gtm_long_t *value, gtm_int_t *err_num)
{
	if (3 != argc)
		return (gtm_status_t)-argc;
	errno = 0;
	if (0 <= (*value = (gtm_long_t)sysconf(name)))	/* Warning - assignment */
		*err_num = 0;
	else
		*err_num = (0 == errno) ? 0 : (gtm_int_t)errno;
	return (gtm_status_t)*err_num;
}

/* posix_syslog() does not entirely follow the format of POSIX syslog(). For one thing, syslog() provides for a
 * variable number of arguments, whereas posix_syslog() can only accommodate a fixed number. Additionally, per
 * http://lab.gsi.dit.upm.es/semanticwiki/index.php/Category:String_Format_Overflow_in_syslog(), the safe way to
 * use syslog() is to force the format to "%s". Note that while POSIX syslog() returns no value, posix_syslog()
 * returns 0; otherwise, GT.M will raise a runtime error.
 */
gtm_status_t posix_syslog(int argc, gtm_int_t priority, gtm_char_t *message)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	/* syslog() is a void function */
	syslog((int)priority, "%s", (char *)message);
	return (gtm_status_t)0;
}

gtm_status_t posix_umask(int argc, gtm_int_t mode, gtm_int_t *prev_mode, gtm_int_t *err_num)
{
	if (3 != argc)
		return (gtm_status_t)-argc;
	*prev_mode = (gtm_int_t)umask(mode);
	return 0;
}

gtm_status_t posix_unsetenv(int argc, gtm_char_t *name, gtm_int_t *err_num)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	*err_num = (-1 == unsetenv(name)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

gtm_status_t posix_utimes(int argc, gtm_char_t *file, gtm_int_t *err_num)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	*err_num = (-1 == utimes(file, NULL)) ? (gtm_int_t)errno : 0;
	return (gtm_status_t)*err_num;
}

/* Helper routines */

/* Given a clock name, provide the numeric value */
gtm_status_t posixhelper_clockval(int argc, gtm_char_t *symconst, gtm_int_t *symval)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	return posixutil_searchstrtab(clocks, clock_values, sizeof(clocks) / sizeof(clocks[0]), symconst, symval);
}

/* Given a symbolic constant for file mode, provide the numeric value */
gtm_status_t posixhelper_filemodeconst(int argc, gtm_char_t *symconst, gtm_int_t *symval)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	return posixutil_searchstrtab(fmodes, fmode_values, sizeof(fmodes) / sizeof(fmodes[0]), symconst, symval);
}

/* Given a symbolic constant for regex facility or level, provide the numeric value */
gtm_status_t posixhelper_regconst(int argc, gtm_char_t *symconst, gtm_int_t *symval)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	return posixutil_searchstrtab(regxflags, regxflag_values, sizeof(regxflags) / sizeof(regxflags[0]), symconst, symval);
}

/* Endian independent conversion from regmatch_t bytestring to offsets */
gtm_status_t posixhelper_regofft2offsets(int argc, gtm_string_t *regofftbytes, gtm_int_t *rmso, gtm_int_t *rmeo)
{
	regmatch_t buf;

	if (3 != argc)
		return (gtm_status_t)-argc;
	memcpy(&buf, regofftbytes->address, sizeof(regmatch_t));
	*rmso = (gtm_int_t)((regoff_t)(buf.rm_so));
	*rmeo = (gtm_int_t)((regoff_t)(buf.rm_eo));
	return 0;
}

/* Given a signal name, provide the numeric value */
gtm_status_t posixhelper_signalval(int argc, gtm_char_t *symconst, gtm_int_t *symval)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	return posixutil_searchstrtab(signals, signal_values, sizeof(signals) / sizeof(signals[0]), symconst, symval);
}

/* Given a configuration name, provide the numeric value */
gtm_status_t posixhelper_sysconfval(int argc, gtm_char_t *symconst, gtm_int_t *symval)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	return posixutil_searchstrtab(sysconfs, sysconf_values, sizeof(sysconfs) / sizeof(sysconfs[0]), symconst, symval);
}

/* Given a symbolic constant for syslog facility or level, provide the numeric value */
gtm_status_t posixhelper_syslogconst(int argc, gtm_char_t *symconst, gtm_int_t *symval)
{
	if (2 != argc)
		return (gtm_status_t)-argc;
	return posixutil_searchstrtab(priority, priority_values, sizeof(priority) / sizeof(priority[0]), symconst, symval);
}

/* Utility routines used by other functions above */

gtm_status_t posixutil_searchstrtab(char *tblstr[], gtm_int_t tblval[], gtm_int_t tblsize, char *str, gtm_int_t *strval)
{
	gtm_int_t compflag, current, first, last;

	first = 0;
	last = tblsize - 1;
	for (; ;)
	{
		current = (first + last) / 2;
		compflag = strcmp(tblstr[current], str);
		if (0 == compflag)
		{
			*strval = tblval[current];
			return (gtm_status_t)0;
		}
		if (first == last)
			return (gtm_status_t)1;
		if (0 > compflag)
			first = (first == current) ? (current + 1) : current;
		else
			last = current;
	}
}
