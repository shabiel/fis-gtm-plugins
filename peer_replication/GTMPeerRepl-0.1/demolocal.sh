#!/bin/bash
# No claim of copyright is made with respect to this shell script
# local test/demo of GT.M peer replication
# remote test/demo would use GT.CM in global directories
# and start jobs remotely, not just locally
echo "****Demo of GT.M Peer Replication"
rm -f help_full.txt
. ./gtmenv
export timestamp=`date -u +%Y-%m-%d_%H:%M:%S`
echo '$timestamp='"$timestamp UTC"
export outfiles=$PWD/outfiles_$timestamp peer1=$PWD/peer1_$timestamp peer2=$PWD/peer2_$timestamp peer3=$PWD/peer3_$timestamp
mkdir -p $outfiles $peer1 $peer2 $peer3
cd $peer1
$gtm_dist/mumps -run GDE >outgde.txt 2>&1 <<EOF
change -segment DEFAULT  -block_size=4096 -allocation=5000 -extension=10000 -global_buffer_count=1000 -file_name=$peer1/gtm.dat
change -region DEFAULT -stdnull -key_size=1019 -record_size=4080 -journal=(before,file="$peer1/gtm.mjl")
exit
EOF
$gtm_dist/mupip create >outcreate.txt 2>&1
cd $peer2
$gtm_dist/mumps -run GDE >outgde.txt 2>&1 <<EOF
change -segment DEFAULT  -block_size=4096 -allocation=5000 -extension=10000 -global_buffer_count=1000 -file_name=$peer2/gtm.dat
change -region DEFAULT -stdnull -key_size=1019 -record_size=4080 -journal=(before,file="$peer2/gtm.mjl")
exit
EOF
$gtm_dist/mupip create >outcreate.txt 2>&1
cd $peer3
$gtm_dist/mumps -run GDE >outgde.txt 2>&1 <<EOF
change -segment DEFAULT  -block_size=4096 -allocation=5000 -extension=10000 -global_buffer_count=1000 -file_name=$peer3/gtm.dat
change -region DEFAULT -stdnull -key_size=1019 -record_size=4080 -journal=(before,file="$peer3/gtm.mjl")
exit
EOF
$gtm_dist/mupip create >outcreate.txt 2>&1
cd ..
$gtm_dist/mumps -run GTMPeerReplDemo $timestamp
stty sane
echo "****Confirm that there are no remaining processes in any peer"
sleep 2 # give mumps processes an opportunity to terminate
cd $peer1 ; $gtm_dist/mupip rundown -region "*"
cd $peer2 ; $gtm_dist/mupip rundown -region "*"
cd $peer3 ; $gtm_dist/mupip rundown -region "*"
cd ..
