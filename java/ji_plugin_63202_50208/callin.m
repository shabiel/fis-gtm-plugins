;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;								;
;	Copyright 2013 Fidelity Information Services, Inc	;
;								;
;	This source code contains the intellectual property	;
;	of its copyright holder(s), and is made available	;
;	under a license.  If you do not know the terms of	;
;	the license, please stop and do not read further.	;
;								;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
callin(boolInVar,intInVar,longInVar,floatInVar,doubleInVar,stringInVar,javaStringInVar,byteArrayInVar,bigDecimalInVar)
	set boolInVar=10,intInVar=1,longInVar=2,floatInVar=3,doubleInVar=4,stringInVar=5,javaStringInVar=6,byteArrayInVar=7,bigDecimalInVar=8
	quit

callinIO(boolInVar,intInVar,longInVar,floatInVar,doubleInVar,stringInVar,javaStringInVar,byteArrayInVar)
	set boolInVar=123,intInVar=123,longInVar=234000000000,floatInVar=345,stringInVar="567",javaStringInVar="678",byteArrayInVar=$char(55)_$char(56)_$char(57)
	quit 123
