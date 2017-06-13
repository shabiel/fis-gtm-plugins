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
package com.fis.gtm.ji;

public class GTMByteArray extends GTMContainer {
	public byte[] value;

	public GTMByteArray(byte[] value) {
		type = GTMContainerType.GTM_BYTE_ARRAY;
		this.value = value;
	}

	@Override
	public String toString() {
		return new String(value);
	}
}
