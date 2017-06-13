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

import java.math.BigDecimal;

import com.fis.gtm.ji.GTMBoolean;
import com.fis.gtm.ji.GTMCI;
import com.fis.gtm.ji.GTMDouble;
import com.fis.gtm.ji.GTMFloat;
import com.fis.gtm.ji.GTMInteger;
import com.fis.gtm.ji.GTMLong;
import com.fis.gtm.ji.GTMString;

public class CallinTest {

	public static void main(String[] args) {
		try {
			boolean booleanValue = false;
			int intValue = 3;
			long longValue = 3141L;
			float floatValue = 3.141f;
			double doubleValue = 3.1415926535;
			String gtmStringValue = "GT.M String Value";
			String javaStringValue = "Java String Value";
			String byteArrayValue = new String(new byte[]{51, 49, 52, 49});
			String bigDecimalValue = "3.14159265358979323846";

			GTMBoolean gtmBoolean = new GTMBoolean(booleanValue);
			GTMInteger gtmInteger = new GTMInteger(intValue);
			GTMLong gtmLong = new GTMLong(longValue);
			GTMFloat gtmFloat = new GTMFloat(floatValue);
			GTMDouble gtmDouble = new GTMDouble(doubleValue);
			GTMString gtmString = new GTMString(gtmStringValue);
			String javaString = javaStringValue;
			byte[] javaByteArray = byteArrayValue.getBytes();
			BigDecimal bigDecimal = new BigDecimal(bigDecimalValue);

			GTMCI.doVoidJob(
				"callin",
				gtmBoolean,
				gtmInteger,
				gtmLong,
				gtmFloat,
				gtmDouble,
				gtmString,
				javaString,
				javaByteArray,
				bigDecimal);

			if (	gtmBoolean.value != booleanValue ||
				gtmInteger.value != intValue ||
				gtmLong.value != longValue ||
				gtmFloat.value != floatValue ||
				gtmDouble.value != doubleValue ||
				!gtmString.value.equals(gtmStringValue) ||
				!javaString.equals(javaStringValue) ||
				!(new String(javaByteArray).equals(byteArrayValue)) ||
				!bigDecimal.toString().equals(bigDecimalValue))
				throw new Exception("GTMJI-INSTALL-ERROR: Call-ins test failed!");

			long longReturnValue = GTMCI.doLongJob(
				"callinIO",
				gtmBoolean,
				gtmInteger,
				gtmLong,
				gtmFloat,
				gtmDouble,
				gtmString,
				javaString,
				javaByteArray);

			if (	longReturnValue != 123 ||
				gtmBoolean.value == booleanValue ||
				gtmInteger.value == intValue ||
				gtmLong.value == longValue ||
				gtmFloat.value == floatValue ||
				gtmDouble.value != doubleValue ||
				gtmString.value.equals(gtmStringValue) ||
				!javaString.equals(javaStringValue) ||
				(new String(javaByteArray).equals(byteArrayValue)))
				throw new Exception("GTMJI-INSTALL-ERROR: Call-ins test failed!");

			System.out.println("GTMJI-INSTALL-SUCCESS: Call-ins test succeeded.");
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}
	}
}

