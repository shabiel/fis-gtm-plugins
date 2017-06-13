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
package com.fis.test;

import com.fis.gtm.ji.GTMBoolean;
import com.fis.gtm.ji.GTMByteArray;
import com.fis.gtm.ji.GTMCI;
import com.fis.gtm.ji.GTMDouble;
import com.fis.gtm.ji.GTMFloat;
import com.fis.gtm.ji.GTMInteger;
import com.fis.gtm.ji.GTMLong;
import com.fis.gtm.ji.GTMString;

public class CalloutTest {
	public static void xcall(Object[] args) {
		GTMBoolean b = (GTMBoolean)args[0];
		GTMInteger i = (GTMInteger)args[1];
		GTMLong l = (GTMLong)args[2];
		GTMFloat f = (GTMFloat)args[3];
		GTMDouble d = (GTMDouble)args[4];
		String s = (String)args[5];
		byte[] a = (byte[])args[6];

		b.value = false;
		i.value = 3;
		l.value = 141;
		f.value = .5926f;
		d.value = 5.358979;
		s = "323846";
		a[0] = (byte)32;
	}

	public static long xcallIO(Object[] args) {
		GTMBoolean b = (GTMBoolean)args[0];
		GTMInteger i = (GTMInteger)args[1];
		GTMLong l = (GTMLong)args[2];
		GTMFloat f = (GTMFloat)args[3];
		GTMDouble d = (GTMDouble)args[4];
		String s = (String)args[5];
		GTMString j = (GTMString)args[6];
		GTMByteArray a = (GTMByteArray)args[7];

		b.value = true;
		i.value = 3;
		l.value = 141000000000L;
		f.value = .5926f;
		d.value = 5.358979;
		s = "323846";
		j.value = "323846";
		a.value = new byte[]{51, 32, 51};

		return 123;
	}
}
