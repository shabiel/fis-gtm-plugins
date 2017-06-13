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
#ifndef __XC_GATEWAY_H
#define __XC_GATEWAY_H

void terminate(void);
long do_job(int count, char *type_descr_ptr, char *class, char *method, va_list var);
long gtm_xcj(int count, char *type_descr_ptr, char *class, char *method, ...);

#endif
