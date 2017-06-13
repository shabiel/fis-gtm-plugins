threeen1glo
	; Find the maximum cycle length for the 3n+1 problem for all integers from 1 through the input.
	; No claim of copyright is made with respect to this code.

	new endat,finish,k,max,n,sets,startat,timestamp	; note: sets is set by subprogram
	set timestamp=$zdate($horolog,"YEAR-MM-DD 24:60:SS")
	read finish if $zeof!(0=$length(finish)) write $text(+0)_"-E-NOINPUT No input provided",! zhalt 255
	set finish=$justify(finish,0,0) if 0'<finish write $text(+0)_"-E-BADINPUT Input must be positive value >0",! zhalt 254
	do
	. write $zcmdline," ",$zversion," ",timestamp," ",$text(+0)," ",$fnumber(finish,",")
	. set startat=$zut,max=$$docycle(finish),endat=$zut
	. write " ",$fnumber(max,",")," ",$fnumber(endat-startat/1E6,",",6)," seconds; "
	. set (k,n)=finish for  set n=$order(^cycle(n)) quit:""=n  if $increment(k) ; count number of nodes
	. write $fnumber(k-1,",")," nodes; ",$fnumber(sets,",")," sets]",!
	quit

docycle(last)
	new i,j,k,currpath,current,maxcycle,n
	for current=1:1:last do
	. set n=current
	. kill currpath
	. set ^cycle(1)=1
	. for i=0:1 quit:$data(^cycle(n))  set currpath(i)=n,n=$select(n#2:3*n+1,1:n/2)
	. do:0<i
	. . if $increment(i,$select(1=n:1,1:^cycle(n)))
	. . set n=""
	. . for  set n=$order(currpath(n)) quit:""=n  set:$increment(sets) ^cycle(currpath(n))=i-n
	set maxcycle=1
	for i=2:1:last set:maxcycle<^cycle(i) maxcycle=^cycle(i)
	quit maxcycle
