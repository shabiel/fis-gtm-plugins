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
start
	set bool=0,int=123,long=456,float=789,double=1011,string1="1213",string2="1415",bytearray="1617"
	set class="com/fis/test/CalloutTest"

xcall
	set iBool=bool,iInt=int,iLong=long,iFloat=float,iDouble=double,iString=string1,iByteArray=bytearray
	do &gtmm2j.xcall(class,"xcall",iBool,iInt,iLong,iFloat,iDouble,iString,iByteArray)
	if ((iBool'=bool)!(iInt'=int)!(iLong'=long)!(iFloat'=float)!(iDouble'=double)!(iString'=string1)!(iByteArray'=bytearray)) goto exception

xcallIO
	set ioBool=bool,ioInt=int,ioLong=long,ioFloat=float,ioDouble=double,ioString1=string1,ioString2=string2,ioByteArray=bytearray
	set ret=$&gtmm2j.xcallIO(class,"xcallIO",.ioBool,.ioInt,.ioLong,.ioFloat,.ioDouble,.ioString1,.ioString2,.ioByteArray)
	if ((ret'=123)!(ioBool=bool)!(ioInt=int)!(ioLong=long)!(ioFloat=float)!(ioDouble=double)!(ioString1'=string1)!(ioString2=string2)!(ioByteArray=bytearray)) goto exception

success
	write "GTMJI-INSTALL-SUCCESS: Call-outs test succeeded.",!
	quit

exception
	write "GTMJI-INSTALL-ERROR: Call-outs test failed!",!
	quit
