;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;								;
;	Copyright 2012 Fidelity Information Services, Inc.    	;
;								;
;	This source code contains the intellectual property	;
;	of its copyright holder(s), and is made available	;
;	under a license.  If you do not know the terms of	;
;	the license, please stop and do not read further.	;
;								;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%ZLIB	; High level wrappers to low level POSIX functions
	quit:$quit 255 quit       ; must call an entryref

compress2(src,dest,level)
	new status
	set:'$length($get(level))!(level>9)!(level<-1) level=-1
	set status=$&gtmzlib.compress2(src,.dest,level)
	quit status

uncompress(src,dest)
	new status
	set status=$&gtmzlib.uncompress(src,.dest)
	quit status

; Provide a version number for this wrapper based on the CVS check in timestamp
VERSION() quit $$version
version()
	new tmp,tmp1,tmp2,tmp3
	set tmp="$Date: 2012/02/21 14:32:09 $" set:8=$length(tmp) tmp="$Date: "_$zdate($horolog,"YYYY/MM/DD 24:60:SS")_" $"
	set tmp1=$piece(tmp," ",2),tmp1=$piece(tmp1,"/",2,3)_"/"_$piece(tmp1,"/",1)
	set tmp2=$piece(tmp," ",3),tmp3=$piece(tmp2,":",3),tmp2=$piece(tmp2,":",1)#13_":"_$piece(tmp2,":",2)_$select(tmp2\13:"PM",1:"AM")
	quit $$FUNC^%DATE(tmp1)_","_(tmp3+$$FUNC^%TI(tmp2))

ZLIBVERSION()  quit $$zlibVersion
zlibversion()  quit $$zlibVersion
zlibVersion()  ; actual API from zlib
	new tmp
	do &gtmzlib.zlibVersion(.tmp)
	quit tmp

;	Error message texts
