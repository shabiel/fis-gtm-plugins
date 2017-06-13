;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;								;
; Copyright (c) 2016 Fidelity National Information       	;
; Services, Inc. and/or its subsidiaries. All rights reserved.	;
;								;
;	This source code contains the intellectual property	;
;	of its copyright holder(s), and is made available	;
;	under a license.  If you do not know the terms of	;
;	the license, please stop and do not read further.	;
;								;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GTMPeerRepl
	;GT.M Peer Replication - n way database peer replication
	;Usage:
	; mumps -run GTMPeerRepl [option] ... where options are:
	; --hang=time - when there is no data to be replicated, how many
	;   milliseconds to wait before checking again; default is 100
	;   and zero means to keep checking in a tight loop
	; --help[=full] - print helpful information and exit without
	;   processing any following options
	; --[loca]lname=instname - name of this instance; instance names
	;   must start with a letter and are case-sensitive
	; --[pull[from]|push[to]]=instname[,gbldirname] - remote instance
	;   name and access information
	; --[purg]e[=time] - purge data recorded before time (measured
	;   as the number of microseconds since the UNIX epoch); 0
	;   means all data recorded; a negative value is relative to
	;   the current time; default is all data already replicated
	;   to known instances
	; --[stat]us[=instname[,...]] - show replication status of named
	;   instances; default to all instances
	; --stop[repl][=instname[,...]] - stop replicating to/from named
	;   instances; default to all instances
	; --[vers]ion - print the version and exit without processing any
	;   following options
	; Options are processed from left to right.
	;
	; Theory of Operation
	;
	; Using triggers, GTMPeerRepl supplements code that captures global
	; variable updates that need to be replicated in nodes of the form
	; ^%YGTMPeerRepl(seq,op,reference)=$ztvalue when op is "S" and
	; ^%YGTMPeerRepl(seq,op,reference)="" when op is "K" or "ZK". seq
	; is a monotonically increasing, but not necessarily consecutive,
	; unique sequence number.
	;
	; For each remote instance to which GTMPeerRepl should push data,
	; or from which GTMPeerRepl should pull data, GTMPeerRepl JOBs a
	; process.  When pushing data to a remote instance, the JOB'd
	; process monitors the end of ^%YGTMPeerRepl() on the local
	; instance. As new data appears, it updates the database on the
	; remote instance identified by the global directory associated
	; with the remote instance name. When pulling data from a remote
	; instance, the JOB'd process monitors the end of ^%YGTMPeerRepl()
	; on the remote instance, and as new data appears, it updates the
	; database on the local instance. If the remote instance is on
	; another node, GT.CM GNP provides the needed access. Note that
	; if the remote instance is on the same node on which GTMPeerRepl
	; is running, both instances must run the same version of GT.M.
	;
	; When resuming replication, the typical case is for the side
	; generating updates to be ahead of the side receiving
	; updates. However, this is not guaranteed - for example, the side
	; generating updates may be a restored backup which is not up to
	; date.  Using a simple sequence number brings up the quandry of
	; where to resume replication - at the sequence number saved on the
	; remote instance or that stored on the local instance. Therefore,
	; for seq, GTMPeerRepl uses $zut, the number of microseconds since
	; the UNIX epoch. Since different processes can generate updates
	; with the same $zut, in the event a node already exists with the
	; $zut that an updating process wishes to use, the example trigger
	; increments the existing highest seq by .01
	;
	; Sample triggers (V6.2-002 & up) that generate information for
	; GTMPeerRepl to replicate are below where each <gbl> specifies a
	; global variable selection to be replicated.  If there is
	; bi-directional replication between instances, there needs to be a
	; way to break infinite loops. The sample triggers use $ztwormhole
	; to break loops - GTMPeerRepl push and pull processes would
	; have $ztwormhole set to "GTMPeerRepl", and application
	; processes would either not set it, or set it to a different
	; value.  GTMPeerRepl would not. As ^%YGTMPeerRepl has numeric
	; and string subscripts, it is required to find the last
	; numeric subscript with $order(), not the last subscript;
	; hence the use of $c(0).
	;
	; Since $zut is a very large number, to make for more readable
	; demonstrations, the example subtracts 1461168335000000 from $zut.
	; This value is completely arbitrary, but all peers must agree on
	; the offset. If you change it, you *must* to a global search and
	; replace throughout the code.
	;
trigdef	; +<gbl> -commands=S -xecute="I ""GTMPeerRepl""'=$ZTWO S tmp1=$R,tmp2=$O(^%YGTMPeerRepl($c(0)),-1),tmp3=$zut-1461168335000000,^%YGTMPeerRepl($S(tmp3>tmp2:tmp3,1:.01+tmp2),$ZTRI,tmp1)=$ZTVA"
	; +<gbl> -commands=K,ZK -xecute="I ""GTMPeerRepl""'=$ZTWO S tmp1=$R,tmp2=$O(^%YGTMPeerRepl($c(0)),-1),tmp3=$zut-1461168335000000,^%YGTMPeerRepl($S(tmp3>tmp2:tmp3,1:.01+tmp2),$ZTRI,tmp1)="""""
	;
	; As triggers execute transactionally, the sample triggers ensure
	; seq is unique and monotonically increasing, at the cost of
	; transaction restarts when collisions occur. For application
	; code that does not use transaction processing, these restarts,
	; when they occur, would be lightweight. For application code
	; that uses transaction processing commands (TSTART/TCOMMIT), in
	; the event transaction restarts become material, other means
	; will be required to (a) modify the application to provide
	; unique, monontonically increasing sequence numbers, (b) modify
	; the application avoid collisions, or (c) enhance GTMPeerRepl to
	; not require unique, monotonically, increasing seq values.
	;
	; Triggers are one way, but not the only way, to generate the
	; nodes in %YGTMPeerRepl. Applications on GT.M versions that
	; predate triggers, or which do not use triggers can use
	; application code to generate the nodes in ^%YGTMPeerRepl that
	; GTMPeerRepl uses.
	;
	; Meta-data (status information) is stored in nodes of
	; ^%YGTMPeerRepl whose first subscript is the name of the current
	; instance, which must start with letter, and is hence
	; non-numeric.
	;; Two semi-colons indicate end of text to be printed for --help=full

	; Set error handler to print error message and return error code to shell
	; (should write to stderr, but GT.M doesn't support that).
	new cmdline,etrap,etrapsav,hngtim,i,localname,nextparm,nextval,rgn,tmp
	set etrap=""
	for i=1:1 set tmp=$text(etrap1+i),tmp=$piece(tmp,";",2,$length(tmp,";")) quit:";"=$extract(tmp,1)  set etrap=etrap_tmp
	set $etrap=etrap
	set:$stack $ecode=",U255,"	; top level entryref only supported when called with mumps -run
	use $principal:(ctrap=$char(3):exception="if $zjobexam()")
	view "jobpid":1	; make JOB'd processes have unique stdout and stderr files

	; See if a local name is set - but it may not be set, or database may not exist, or global directory may not exist
	; Set $etrap to ignore database file missing error, should $view() issue it
	set etrapsav=$etrap,etrap=""
	for i=1:1 set tmp=$text(etrap2+i),tmp=$piece(tmp,";",2,$length(tmp,";")) quit:";"=$extract(tmp,1)  set etrap=etrap_tmp
	set $etrap=etrap
	set:""'=$zsearch($zgbldir)&(""'=$view("gvfile",$view("region","^%YGTMPeerRepl"))) localname=$get(^%YGTMPeerRepl("localname"))
	set:'$data(localname) localname=""	    	; if not in the database, make localname the null string so it is not undefined later
	set $etrap=etrapsav	; restore $etrap

	; Process command line and exit
	set cmdline=$zcmdline
	set hngtim=.1					; default hang time is 100 msec
	for  quit:'$$trimleadingstr(.cmdline,"--")  do	; process command line options
	. set tmp=$$trimleadingpiece(.cmdline," ")  	; tmp contains next parameter including value
	. set nextparm=$piece(tmp,"=",1)	    	; nextparm contains next paramter keyword
	. set nextval=$piece(tmp,"=",2)			; nextval contains next parameter value, if any
	. if $$matchparm(nextparm,"hang","hang") set:'$length(nextval) $ecode=",U247," set hngtim=+nextval/1000
	. else  if $$matchparm(nextparm,"help","help") do help(nextval) ; print help and exit - never returns here
	. else  if $$matchparm(nextparm,"localname","loca") set:'(nextval?1A.AN) $ecode=",U252," set (localname,^%YGTMPeerRepl("localname"))=nextval
	. else  if $$matchparm(nextparm,"pullfrom","pull")!$$matchparm(nextparm,"pushto","push") do startrepl
	. else  if $$matchparm(nextparm,"purge","purg") do purge
	. else  if $$matchparm(nextparm,"status","stat") do status
	. else  if $$matchparm(nextparm,"stoprepl","stop") do stoprepl
	. else  if $$matchparm(nextparm,"version","vers") write $text(+0)," ",$piece($text(version),";",2,$length($text(version),";")),! halt
	quit

; The labels below are not intended to be called from external programs, and hence
; reference local variables set in the main program and not passed as parameters.

ensurelocalname
	; terminate with error if localname not set
	set:'$length($get(localname)) $ecode=",U249,"	; localname must be defined
	quit

etrap1	; Long line of code for standard $etrap broken into readable chunks
	;set $etrap="use $principal write $zstatus,! zhalt 1"
	; set tmp1=$piece($ecode,",",2),tmp2=$text(@tmp1)
	; if $length(tmp2) zshow "S":tmp3
	; write $piece(tmp3("S",1)," ",1),": ",$text(+0),@$piece(tmp2,";",2,$length(tmp2,";")),!
	; zhalt +$extract(tmp1,2,$length(tmp1))
	;;

etrap2	; Long line of code for special $etrap when searching for localname broken into readble chunks
	;set $etrap="use $principal write $zstatus,! zhalt 1"
	; zshow "S":tmp1 set tmp1=$piece(tmp1("S",1)," ",1)
	; set tmp2=$piece(tmp1,"+",1)_"+"_(1+$piece(tmp1,"+",2))_"^"_$piece(tmp1,"^",2)
	; set:"Z150372546"=$piece($ecode,",",2) ($ecode,$zstatus)=""
	; zgoto @($zlevel_":"_tmp2)
	;;

help(detail)
	; print help and exit
	new i,rtn,tmp1,tmp2,tmp3
	set detail=$select('$data(detail):"",1:$zconvert(detail,"L"))
	set rtn=$text(+0)
	for i=1:1 set tmp1=rtn_"+"_i_"^"_rtn,tmp2=$text(@tmp1),tmp3=$piece(tmp2,";",2,$length(tmp2,";")) quit:$select("full"=detail:2=$length(tmp2,";;"),1:'$length(tmp3))  write tmp3,!
	halt

matchparm(s,x,y)
	; Return whether s matches a minimum abbreviation of x specified by y
	quit y=$extract(s,1,$length(y))&(s=$extract(x,1,$length(s)))

pull(instcurr,instfrom,hngtim)
	; look for updates on remote instance, and pull to local instance
	; instcurr - current instance name
	; instfrom - name of remote instance
	; hngtim - time to hang when there is no data to push
	new gbldir,lupd,lupdrem,nxtseq,op,quitflag,tmp,var,varrem
	; gbldir - global directory of remote instance
	; lstupd - last update replicated
	; lstupdrem - last update replicate per remote database
	; nxtupdrem - next update to replicate from remote database
	; op - operation to replicate (S/K/ZK)
	; quitflag - set by $zinterrupt when process told to quit
	; tmp -
	; var - variable name for replicated operation
	; varrem - variable name prefixed by global directory of remote instance
	set quitflag=0,$zinterrupt="set quitflag=1"	; setup quit on interrupt
	set gbldir=$zsearch(^%YGTMPeerRepl(instcurr,instfrom,"gbldir"))	; assert $zsearch() context is clean - JOB'd process starts at pull()
	set:'$length(gbldir) $ecode=",U254,"				; couldn't get global directory for remote instance
	; status subscripts are local,remote,...
	set tmp=$zut-1461168335000000,^%YGTMPeerRepl(instcurr,instfrom,tmp)="startpull,"_$job,^|gbldir|%YGTMPeerRepl(instfrom,instcurr,tmp)="sendpull,"_$job
	; lock subscripts are origin,destination to ensure only one process is replicating from one instance to another
	lock +(^%YGTMPeerRepl(instfrom,instcurr),^|gbldir|%YGTMPeerRepl(instfrom,instcurr)):0
	set:'$test $ecode=",U253,"	; some other process is already replicating
	set $ztwormhole="GTMPeerRepl"	; set $ztwormhole to prevent infinite loops
	; Start replication from later of local and remote last updates, continue until interrupted when xecute $zinterrupt sets quitflag
	; Local update later than remote can occur if remote restored from backup, log if this is the case
	set lstupd=+$get(^%YGTMPeerRepl(instcurr,instfrom,"seqfrom"))
	set lstupdrem=+$get(^|gbldir|%YGTMPeerRepl(instfrom,instcurr,"seqto"))
	set:lstupdrem<lstupd tmp=$zut-1461168335000000,(^%YGTMPeerRepl(instcurr,instfrom,tmp),^|gbldir|%YGTMPeerRepl(instfrom,instcurr,tmp))="ambigpull,"_lstupdrem_","_lstupd,(^|gbldir|%YGTMPeerRepl(instfrom,instcurr,"seqto"),lstupdrem)=lstupd
	; As ^%YGTMPeerRepl has status information in nodes with non-numeric first subscript and updates
	; replicate in nodes with numeric first subscript, nxtupd must be coerced to numeric
	for  quit:$data(^%YGTMPeerRepl(instcurr,instfrom,"stop"))!$data(^|gbldir|%YGTMPeerRepl(instfrom,instcurr,"stop"))  set nxtupdrem=+$order(^|gbldir|%YGTMPeerRepl(lstupdrem)) do:nxtupdrem  hang:'nxtupdrem hngtim
	. set op=$order(^|gbldir|%YGTMPeerRepl(nxtupdrem,"")),var=$order(^|gbldir|%YGTMPeerRepl(nxtupdrem,op,"")),varrem="^|"""_gbldir_"""|"_$extract(var,2,$length(var))
	. ; Modify the following code if the application should do something more sophisticated than just pulling updates,
	. ; e.g., to check if a node to be set already exists, and to generate an error
	. if "S"=op set @var=^|gbldir|%YGTMPeerRepl(nxtupdrem,op,var)
	. else  if "K"=op kill @var
	. else  zkill @var ; assert "ZK"=op - only supported operations are S, K and ZK
	. set (^%YGTMPeerRepl(instcurr,instfrom,"seqfrom"),^|gbldir|%YGTMPeerRepl(instfrom,instcurr,"seqto"),lstupd)=nxtupdrem
	; Clean up and exit
	lock -(^%YGTMPeerRepl(instfrom,instcurr),^|gbldir|%YGTMPeerRepl(instfrom,instcurr))
	set tmp=$zut-1461168335000000,%YGTMPeerRepl(instcurr,instfrom,tmp)="stoppull,"_$job,^|gbldir|%YGTMPeerRepl(instfrom,instcurr,tmp)="donepull,"_$job
	quit

purge	; purge data collected by triggers
	new peer,when,seq,tmp,tmp1,tmp2
	set when=$zut-1461168335000000	 			; default is all data prior to this instant
	; unspecified nextval means through latest data replicated to all known peers
	if '$length(nextval) do ensurelocalname set peer="" for  set peer=$order(^%YGTMPeerRepl(localname,peer)) quit:'$length(peer)  do
	. set tmp1=+$get(^%YGTMPeerRepl(localname,peer,"seqfrom")),tmp2=+$get(^%YGTMPeerRepl(localname,peer,"seqto"))
	. set tmp=$select(tmp2>tmp1:tmp2,1:tmp1)	; tmp is higher sequence number pushed from localname to peer or pulled by peer from localname
	. set:when>tmp when=tmp				; when is lowest sequence number replicated to any peer
    	; specified positive nextval means all data prior to it, negative nextval is relative to current time
	else  set nextval=+nextval set:nextval when=$select(nextval>0:nextval,1:when+nextval)
	set seq=0 for  set seq=$order(^%YGTMPeerRepl(seq)) quit:seq>when!(seq'=+seq)  kill ^%YGTMPeerRepl(seq)
	quit

push(instcurr,instto,hngtim)
	; look for updates on local instance, and push to remote instance
	; instcurr - current instance name
	; instto - name of remote instance
	; hngtim - time to hang when there is no data to push
	new gbldir,lupd,lupdrem,nxtseq,op,quitflag,tmp,var,varrem
	; gbldir - global directory of remote instance
	; lstupd - last update replicated
	; lstupdrem - last update replicate per remote database
	; nxtupd - next update to replicate
	; op - operation to replicate (S/K/ZK)
	; quitflag - set by $zinterrupt when process told to quit
	; tmp -
	; var - variable name for replicated operation
	; varrem - variable name prefixed by global directory of remote instance
	set quitflag=0,$zinterrupt="set quitflag=1"	; setup quit on interrupt
	set gbldir=$zsearch(^%YGTMPeerRepl(instcurr,instto,"gbldir"))	; assert $zsearch() context is clean - JOB'd process starts at push()
	set:'$length(gbldir) $ecode=",U254,"				; couldn't get global directory for remote instance
	; status subscripts are local,remote,...
	set tmp=$zut-1461168335000000,^%YGTMPeerRepl(instcurr,instto,tmp)="startpush,"_$job,^|gbldir|%YGTMPeerRepl(instto,instcurr,tmp)="recvpush,"_$job
	; lock subscripts are origin,destination to ensure only one process is replicating from one instance to another
	lock +(^%YGTMPeerRepl(instcurr,instto),^|gbldir|%YGTMPeerRepl(instcurr,instto)):0
	set:'$test $ecode=",U253,"	; some other process is already replicating
	set $ztwormhole="GTMPeerRepl"	; set $ztwormhole to prevent infinite loops
	; Start replication from later of local and remote last updates, continue until interrupted when xecute $zinterrupt sets quitflag
	; Remote update later than local can occur if local restored from backup, log if this is the case
	set lstupd=+$get(^%YGTMPeerRepl(instcurr,instto,"seqto"))
	set lstupdrem=+$get(^|gbldir|%YGTMPeerRepl(instto,instcurr,"seqfrom"))
	set:lstupdrem>lstupd tmp=$zut-1461168335000000,(^%YGTMPeerRepl(instcurr,instto,tmp),^|gbldir|%YGTMPeerRepl(instto,instcurr,tmp))="ambigpush,"_lstupd_","_lstupdrem,(^%YGTMPeerRepl(instcurr,instto,"seqto"),lstupd)=lstupdrem
	; As ^%YGTMPeerRepl has status information in nodes with non-numeric first subscript and updates
	; replicate in nodes with numeric first subscript, nxtupd must be coerced to numeric
	for  quit:$data(^%YGTMPeerRepl(instcurr,instto,"stop"))!$data(^|gbldir|%YGTMPeerRepl(instto,instcurr,"stop"))  set nxtupd=+$order(^%YGTMPeerRepl(lstupd)) do:nxtupd  hang:'nxtupd hngtim
	. set op=$order(^%YGTMPeerRepl(nxtupd,"")),var=$order(^%YGTMPeerRepl(nxtupd,op,"")),varrem="^|"""_gbldir_"""|"_$extract(var,2,$length(var))
	. ; Modify the following code if the application should do something more sophisticated than just pushing updates,
	. ; e.g., to check if a node to be set already exists, and to generate an error
	. if "S"=op set @varrem=^%YGTMPeerRepl(nxtupd,op,var)
	. else  if "K"=op kill @varrem
	. else  zkill @varrem ; assert "ZK"=op - only supported operations are S, K and ZK
	. set (^%YGTMPeerRepl(instcurr,instto,"seqto"),^|gbldir|%YGTMPeerRepl(instto,instcurr,"seqfrom"),lstupd)=nxtupd
	; Clean up and exit
	lock -(^%YGTMPeerRepl(instcurr,instto),^|gbldir|%YGTMPeerRepl(instcurr,instto))
	set tmp=$zut-1461168335000000,^%YGTMPeerRepl(instcurr,instto,tmp)="stoppush,"_$job,^|gbldir|%YGTMPeerRepl(instto,instcurr,tmp)="donepush,"_$job
	quit

startrepl
	; process command line argument to push or pull
	new gbldir,remotename,tmp
	do ensurelocalname
	set remotename=$piece(nextval,",",1)
	set:'(remotename?1A.AN) $ecode=",U251,"
	set tmp=$piece(nextval,",",2),gbldir=$select($length(tmp):tmp,1:$get(^%YGTMPeerRepl(localname,remotename,"gbldir")))
	set:$length($zsearch("/dev/null")) tmp=$zsearch(gbldir) ; must reset $zsearch() context before validating global directory
	if $length(tmp) set (gbldir,^%YGTMPeerRepl(localname,remotename,"gbldir"))=tmp	; no-op duplicate set if gbldir came from ^%YGTMPeerRepl
	else  set $ecode=",U250,"	; error - invalid global directory for remote instance
	kill ^%GTMPeerRepl(localname,remotename,"stop"),^|gbldir|%YGTMPeerRepl(remotename,localname,"stop")
	job @nextparm@(localname,remotename,hngtim)	 ; start replication with pull or push job
	quit

status	; report the current state of replication
	new dir,event,i,lastseq,next,peers,quitflag,seq,tmp1
	do ensurelocalname
	set:'$data(nextval) nextval=""		; make sure nextval is defined
	write "Localname=",localname," Lastseq=",$fnumber($order(^%YGTMPeerRepl($c(0)),-1),","),!
	if '$length($get(nextval)) set next=$c(0),peers="" do  set peers=$extract(peers,1,$length(peers)-1)
	. for  set next=$order(^%YGTMPeerRepl(localname,next)) quit:'$length(next)  set peers=peers_next_","
	else  do validatepeers(localname,nextval) set peers=nextval		; otherwise validate peer names
	for i=1:1:$length(peers,",") set next=$piece(peers,",",i) do
	. write "  Peer=",next
	. write " Sent=",$fnumber(+$get(^%YGTMPeerRepl(localname,next,"seqto")),",")
	. write " Received=",$fnumber(+$get(^%YGTMPeerRepl(localname,next,"seqfrom")),",")
	. set tmp1=$get(^%YGTMPeerRepl(localname,next,"gbldir")) if $length(tmp1) write " Gbldir=",tmp1,!
	. else  write !
	. set seq=$c(0),quitflag=0 for seq=$order(^%YGTMPeerRepl(localname,next,seq),-1) quit:'$length(seq)!quitflag  do
	. . set event=^%YGTMPeerRepl(localname,next,seq)
	. . if event?1"stop".E write "    ",$extract(event,5,$find(event,",")-2)," stopped pid=",$piece(event,",",2)," at ",$fnumber(seq,","),!
	. . else  if event?1"start".E write "    ",$extract(event,6,$find(event,",")-2)," started pid=",$piece(event,",",2)," at ",$fnumber(seq,","),!
	quit

stoprepl
	; process command line argument to stop replication
	new i,j,next,peers,pid,sigusr1,startflag,stopflag,tmp
	do ensurelocalname
	set:'$data(nextval) nextval=""		; make sure nextval is defined
	set tmp=$&gtmposix.signalval("SIGUSR1",.sigusr1) ; get signal value for USR1 to interrupt processes; discard return value
	if '$length($get(nextval)) set next=$c(0),peers="" do  set peers=$extract(peers,1,$length(peers)-1)
	. for  set next=$order(^%YGTMPeerRepl(localname,next)) quit:'$length(next)  set peers=peers_next_","
	else  do validatepeers(localname,nextval) set peers=nextval		; otherwise validate peer names
	if $length(peers) for i=1:1:$length(peers,",") do
	. set next=$piece(peers,",",i),j=$char(0),(startflag,stopflag)=0
	. for  set j=$order(^%YGTMPeerRepl(localname,next,j),-1) quit:'$length(j)  do  quit:stopflag!startflag
	. . if ^%YGTMPeerRepl(localname,next,j)?1"stop".E set stopflag=1
	. . else  if ^%YGTMPeerRepl(localname,next,j)?1"start".E set startflag=1
	. do:$length(j)&startflag	; startflag=1 means push/pull process was started, but not stopped
	. . set pid=$piece(^%YGTMPeerRepl(localname,next,j),",",2)
	. . set (^%GTMPeerRepl(localname,next,"stop"),^|^%YGTMPeerRepl(localname,next,"gbldir")|%YGTMPeerRepl(next,localname,"stop"))=$zut-1461168335000000_","_$job
	quit

trimleadingpiece(s,x)
	; Remove and optionally return first piece of s with x as piece separator
	new tmp
	set tmp=$piece(s,x,1)
	set s=$piece(s,x,2,$length(s,x))
	quit:$quit tmp quit

trimleadingstr(s,x)
	; Return s without leading $length(x) characters; return 1/0 if called as function
	if x=$extract(s,1,$length(x)) set s=$extract(s,$length(x)+1,$length(s)) quit:$quit 1 quit
	else  quit:$quit 0 quit

validatepeers(inst,peers)
	; validate that s contains a comma separated list of valid peer names
	new i,pname
	for i=1:1:$length(peers,",") set pname=$piece(peers,",",i) set:'$data(^%YGTMPeerRepl(inst,pname)) $ecode=",U248,"
	quit	; if the code gets here, all peer names in p appear in the database

version	;0.1

;	Error message texts
M6	;" "_$zstatus
U247	;"-F-UNDEFHNG Hang time must be specified and cannot be defaulted"
U248	;"-F-UNKPEER Peer """_pname_""" unknown to instance """_inst
U249	;"-F-NOLOCNAM Name of local instance available from neither database nor command line"
U250	;"-F-INVGBLDIR Filename="""_gbldir_""" - must match a global directory file"
U251	;"-F-INVINST push/pull instance name="""_instname_""" - must be one alpha followed by alphanumeric characters"
U252	;"-F-INVINST local instance name="""_nextval_""" - must be one alpha followed by alphanumeric characters"
U253	;"-F-NOTUNIQ Unable to ensure uniqueness; check for local and remote holder of LOCK ^%YGTMPeerRepl("_instcurr_","_instto_")"
U254	;"-F-BADGBLDIR ^%YGTMPeerRepl("_instcurr_","_instto_",""gbldir"")="_^%YGTMPeerRepl(instcurr,instto,"gbldir")_" is not a valid file"
U255	;"-F-BADINVOCATION Must invoke from shell as mumps -run "_$text(+0)
