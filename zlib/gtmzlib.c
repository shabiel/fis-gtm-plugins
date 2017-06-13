/****************************************************************
 *								*
 *	Copyright 2012 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include <string.h>
#include <zlib.h>
#include "gtmxc_types.h"

/* Limits - these are GT.M limits; if your application limits are smaller use them for better performance */
#define maxstr 1048576

gtm_status_t zlib_compress2(int argc, gtm_string_t *instr, gtm_string_t *outstr, gtm_int_t level)
{
	outstr->length = maxstr;
	return (gtm_status_t)compress2((Bytef *)outstr->address,
				       (uLongf *)&outstr->length,
				       (Bytef *)instr->address,
				       (uLong)instr->length,
				       (int)level);
}

gtm_status_t zlib_uncompress(int argc, gtm_string_t *instr, gtm_string_t *outstr)
{
	outstr->length = maxstr;
	return (gtm_status_t)uncompress((Bytef *)outstr->address,
				       (uLongf *)&outstr->length,
				       (Bytef *)instr->address,
				       (uLong)instr->length);
}

gtm_status_t zlib_zlibVersion(int argc, gtm_char_t * zlibver)
{
	strncpy(zlibver, zlibVersion(), 256);
	return 0;
}
