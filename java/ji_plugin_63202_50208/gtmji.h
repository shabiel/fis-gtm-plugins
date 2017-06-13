/****************************************************************
*								*
*	Copyright 2013 Fidelity Information Services, Inc	*
*								*
*	This source code contains the intellectual property	*
*	of its copyright holder(s), and is made available	*
*	under a license.  If you do not know the terms of	*
*	the license, please stop and do not read further.	*
*								*
****************************************************************/
#ifndef __GTMJI_H
#define __GTMJI_H

/* GTMJI_VERSION 1.0.1 */

#define MSG_LBL			"GTM-E-JNI, "
#define BUF_LEN			2048
#define MAX_PARAMS		32
#define MAX_STRLEN		1048576

#ifdef DEBUG
# define DEBUG_ONLY(X)		X
/* In case of debug the source file includes assert.h, and so assert() is defined. */
#else
# define DEBUG_ONLY(X)
# define assert(X)
#endif

/* Prints a message prepended by MSG_LBL and terminated with a newline. */
#define MSG(...)		{fprintf(stderr, MSG_LBL); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr);}

#define CUR_JNI_VERSION		JNI_VERSION_1_6

/* Enumeration of all types that can be used with Java call-ins and call-outs. */
enum gtm_jtypes
{
	GTM_BOOLEAN = 0,
	GTM_INTEGER,
	GTM_LONG,
	GTM_FLOAT,
	GTM_DOUBLE,
	GTM_STRING,
	GTM_BYTE_ARRAY,
	GTM_JTYPES_SIZE = 7,
	JAVA_STRING = 7,
	JAVA_BYTE_ARRAY,
	JAVA_BIG_DECIMAL,
	JTYPES_SIZE = 10
};

/* Names of all argument types to be used in error messages. */
STATICDEF const char *arg_type_names[] =
{
	"GTMBoolean",
	"GTMInteger",
	"GTMLong",
	"GTMFloat",
	"GTMDouble",
	"GTMString",
	"GTMByteArray",
	"String",
	"byte[]",
	"BigDecimal"
};

#endif
