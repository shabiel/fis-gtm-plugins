; No claim of copyright is made with respect to this program
GTMPeerReplDemo
	; Demo / test of GT.M Peer Replication
	; Initialization
	new dir,i,input,io,j,k,outfiles,output,peer1,peer2,peer3,pgm,replvar,s,timestamp,trigdef,trigfil1,trigfil2
	set io=$io
	set timestamp=$piece($zcmdline," ",1)
	set outfiles=$zsearch("outfiles_"_timestamp)
	set peer1=$zsearch("peer1_"_timestamp)
	set peer2=$zsearch("peer2_"_timestamp)
	set peer3=$zsearch("peer3_"_timestamp)
	set pgm=$piece($zcmdline," ",2)
	set:""=pgm pgm="threeen1glo"
	; Identify run
	write $text(+0)," ",timestamp," "	; no end of line to allow GTMPeerRepl version
	zsystem "$gtm_dist/mumps -run GTMPeerRepl --version"
	; Get the trigger definitions from routine GTMPeerRepl
	set i=0 for  set s=$piece($text(trigdef+i^GTMPeerRepl),"+<gbl>",2) quit:'$length(s)  set trigdef($increment(i))=s
	; Get the variable(s) to replicate
	do loadfile("trig.txt",.replvar)
	; Generate files with trigger definitions to load and unload
	set trigfile1=outfiles_"/"_$text(+0)_"_"_timestamp_"_"_$job_"_trigload.txt"
	set trigfile2=outfiles_"/"_$text(+0)_"_"_timestamp_"_"_$job_"_trigdel.txt"
	open trigfile1:newversion use trigfile1
	for i=1:1 quit:'$data(trigdef(i))  for j=1:1 quit:'$data(replvar(j))  write "+^",replvar(j),trigdef(i),!
	open trigfile2:newversion use trigfile2 close trigfile1
	for i=1:1 quit:'$data(trigdef(i))  for j=1:1 quit:'$data(replvar(j))  write "-^",replvar(j),trigdef(i),!
	use io close trigfile2
	write "MUPIP TRIGGER file to load triggers is ",trigfile1,!,"MUPIP TRIGGER file to delete triggers is ",trigfile2,!
	; Get the input to feed the programs
	do loadfile("prog_in.txt",.input)
	write "****Checking --help: mumps -run GTMPeerRepl --help",!
	zsystem "$gtm_dist/mumps -run GTMPeerRepl --help"
	write "****Checking --help=full: $gtm_dist/mumps -run GTMPeerRepl --help=full >"_outfiles_"/help_full.txt",!
	zsystem "$gtm_dist/mumps -run GTMPeerRepl --help=full >"_outfiles_"/help_full.txt"
	; Load the triggers
	write "****Loading triggers - outputs in outtrigload.txt files in each peer.",!
	open peer1:(shell="/bin/sh":command="cd "_peer1_" ; $gtm_dist/mupip trigger -triggerfile="_trigfile1_" 1>outtrigload.txt 2>&1":readonly:writeonly)::"pipe"
	use io close peer1
	open peer2:(shell="/bin/sh":command="cd "_peer2_" ; $gtm_dist/mupip trigger -triggerfile="_trigfile1_" 1>outtrigload.txt 2>&1":readonly:writeonly)::"pipe"
	use io close peer2
	open peer3:(shell="/bin/sh":command="cd "_peer3_" ; $gtm_dist/mupip trigger -triggerfile="_trigfile1_" 1>outtrigload.txt 2>&1":readonly:writeonly)::"pipe"
	use io close peer3
	; Turn on replication - peer1 pushes to peer2, peer3 pulls from peer1, peer2 & peer3 push to each other
	write "****Turning on peer replication",!
	; Different hang times used for demonstration purposes & testing randomization
	open peer1:(shell="/bin/sh":command="cd "_peer1_" ; $gtm_dist/mumps -run GTMPeerRepl --loca=peer1 --push=peer2,"_peer2_"/gtm.gld --hang="_(1+$random(10))_" 1>outpush.txt 2>&1":readonly:writeonly)::"pipe"
	close peer1
	open peer2:(shell="/bin/sh":command="cd "_peer2_" ; $gtm_dist/mumps -run GTMPeerRepl --loca=peer2 --push=peer3,"_peer3_"/gtm.gld 1>outpush.txt 2>&1":readonly:writeonly)::"pipe"
	close peer2
	open peer3:(shell="/bin/sh":command="cd "_peer3_" ; $gtm_dist/mumps -run GTMPeerRepl --loca=peer3 --push=peer2,"_peer2_"/gtm.gld --hang="_(1+$random(10))_" 1>outpush.txt 2>&1":readonly:writeonly)::"pipe"
	close peer3
	open peer3:(shell="/bin/sh":command="cd "_peer3_" ; $gtm_dist/mumps -run GTMPeerRepl --pull=peer1,"_peer1_"/gtm.gld 1>outpull.txt 2>&1":readonly:writeonly)::"pipe"
	close peer3
	; Run the peers
	write "****Run peer instances of application",!
	set type="Peer1 pushes to peer2. Peer3 pulls from peer1, peer2 & peer3 push to each other."
	do drivepgms(type)
	hang 1 ; timing dependency here, need something better
	write "****Check status - peer2 is only peer3 status, others are for all peers",!
	; Display status while programs are running
	zsystem "cd "_peer1_" ; $gtm_dist/mumps -run GTMPeerRepl --status"
	zsystem "cd "_peer2_" ; $gtm_dist/mumps -run GTMPeerRepl --status=peer3"
	zsystem "cd "_peer3_" ; $gtm_dist/mumps -run GTMPeerRepl --status"
	; Get and display results
	write "****Wait for programs to complete and get results - number of nodes should be the same for all peers",!
	write "****Number of sets should be same as number of nodes for peer1; and less for other peers",!
	do getresult(type)
	; Turn off replication
	write "****Stop replication - outputs in outstop.txt files in each peer, which should all be empty",!
	open peer1:(shell="/bin/sh":command="cd "_peer1_" ; $gtm_dist/mumps -run GTMPeerRepl --stop=peer2 1>outstop.txt 2>&1":readonly:writeonly)::"pipe" close peer1
	open peer2:(shell="/bin/sh":command="cd "_peer2_" ; $gtm_dist/mumps -run GTMPeerRepl --stop=peer3 1>outstop.txt 2>&1":readonly:writeonly)::"pipe" close peer2
	open peer3:(shell="/bin/sh":command="cd "_peer3_" ; $gtm_dist/mumps -run GTMPeerRepl --stop 1>outstop.txt 2>&1":readonly:writeonly)::"pipe" close peer3
	;
	; Delete the triggers
	write "****Deleting triggers - outputs in outtrigdel.txt files in each peer.",!
;	zsystem "cat "_trigfile2
	open peer1:(shell="/bin/sh":command="cd "_peer1_" ; $gtm_dist/mupip trigger -triggerfile="_trigfile2_" 1>outtrigdel.txt 2>&1":readonly:writeonly)::"pipe"
	close peer1
	open peer2:(shell="/bin/sh":command="cd "_peer2_" ; $gtm_dist/mupip trigger -triggerfile="_trigfile2_" 1>outtrigdel.txt 2>&1":readonly:writeonly)::"pipe"
	close peer2
	open peer3:(shell="/bin/sh":command="cd "_peer3_" ; $gtm_dist/mupip trigger -triggerfile="_trigfile2_" 1>outtrigdel.txt 2>&1":readonly:writeonly)::"pipe"
	close peer3
	;
	; Purge the data for replication - all data for peers 1; last 3 seconds for peer2; only already replicated data for peer3 (just to demonstrate capabilities)
	write "****Space in database before purge of updates to be replicated - outputs in outpurge.txt files in each peer, which should all be empty",!
	set dir=$zdir
	set $zdir=peer1,$zgbldir=$zdir_"/gtm.gld"
	write "peer1: Free/Total=",$view("freeblocks","DEFAULT"),"/",$view("totalblocks","DEFAULT"),!
	set $zdir=peer2,$zgbldir=$zdir_"/gtm.gld"
	write "peer2: Free/Total=",$view("freeblocks","DEFAULT"),"/",$view("totalblocks","DEFAULT"),!
	set $zdir=peer3,$zgbldir=$zdir_"/gtm.gld"
	write "peer3: Free/Total=",$view("freeblocks","DEFAULT"),"/",$view("totalblocks","DEFAULT"),!
	open peer1:(shell="/bin/sh":command="cd "_peer1_" ; $gtm_dist/mumps -run GTMPeerRepl --purge=0 1>outpurge.txt 2>&1":readonly:writeonly)::"pipe" close peer1
	open peer2:(shell="/bin/sh":command="cd "_peer2_" ; $gtm_dist/mumps -run GTMPeerRepl --purge=-3E6 1>outpurge.txt 2>&1":readonly:writeonly)::"pipe" close peer2
	open peer3:(shell="/bin/sh":command="cd "_peer3_" ; $gtm_dist/mumps -run GTMPeerRepl --purge 1>outpurge.txt 2>&1":readonly:writeonly)::"pipe" close peer3
	write "****Space after purge - all data for peers 1; last 3 seconds for peer2; only already replicated data for peer3",!
	set $zdir=peer1,$zgbldir=$zdir_"/gtm.gld"
	write "peer1: Free/Total=",$view("freeblocks","DEFAULT"),"/",$view("totalblocks","DEFAULT"),!
	set $zdir=peer2,$zgbldir=$zdir_"/gtm.gld"
	write "peer2: Free/Total=",$view("freeblocks","DEFAULT"),"/",$view("totalblocks","DEFAULT"),!
	set $zdir=peer3,$zgbldir=$zdir_"/gtm.gld"
	write "peer3: Free/Total=",$view("freeblocks","DEFAULT"),"/",$view("totalblocks","DEFAULT"),!
	set $zdir=dir
	quit

drivepgms(type)	; start peer applications and feed them input
	write type,!
	open peer1:(shell="/bin/sh":command="cd "_peer1_" ; $gtm_dist/mumps -run "_pgm)::"pipe"
	open peer2:(shell="/bin/sh":command="cd "_peer2_" ; $gtm_dist/mumps -run "_pgm)::"pipe"
	open peer3:(shell="/bin/sh":command="cd "_peer2_" ; $gtm_dist/mumps -run "_pgm)::"pipe"
	for i=1:1 quit:'$data(input(i))  do
	. use peer1 write input(i),!
	. use peer2 write input(i),!
	. use peer3 write input(i),!
	use peer1 write /eof
	use peer2 write /eof
	use peer3 write /eof
	use io
	quit

getresult(type)
	kill output(type)
	write "Results",!
	use peer1 for i=1:1 read s quit:$zeof  set output(type,peer1,i)=s
	use peer2 close peer1
	for i=1:1 read s quit:$zeof  set output(type,peer2,i)=s
	use peer3 close peer2
	for i=1:1 read s quit:$zeof  set output(type,peer3,i)=s
	use io close peer3
	write "  peer1:",!
	for i=1:1 quit:'$data(output(type,peer1,i))  write "    ",output(type,peer1,i),!
	write "  peer2:",!
	for i=1:1 quit:'$data(output(type,peer2,i))  write "    ",output(type,peer2,i),!
	write "  peer3:",!
	for i=1:1 quit:'$data(output(type,peer3,i))  write "    ",output(type,peer3,i),!
	quit

loadfile(fname,var) ; read and load lines of a file into a local variable
	new i,io,s
	set io=$io
	open fname:readonly
	use fname for i=1:1 read s quit:$zeof  set var(i)=s
	use io close fname
	quit
