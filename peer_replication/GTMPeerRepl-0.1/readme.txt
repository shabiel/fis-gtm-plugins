Copyright (c) 2016 Fidelity National Information Services, Inc. and/or
its subsidiaries. All rights reserved.

--------------------
REVISION HISTORY
--------------------

  0.1  2016-04-22 Initial Release
  0.11 2016-05-12 Changes from Peer Review

--------------------
COPYRIGHT and LICENSING
--------------------

No claim of copyright is made to the 3n+1 program in threeen1glo.m, or
its input file prog_in.txt. Except where otherwise noted within a
file, all other files in the GT.M Peer Replication package are covered
by the Copyright statement at the beginning of this readme file.

If you receive the software in this package integrated with a GT.M
distribution, the license for your GT.M distribution is also your
license for this plugin.

In the event the package contains a COPYING file, that is your license
for this software. Except as noted below, you must retain that file
and include it if you redistribute the package or a derivative work
thereof.

If you have a signed agreement that legitimately provides you with GT.M
under a different license from that in the COPYING file, you may, at
your option, delete the COPYING file and use this software as an
integral part of GT.M under the same terms as that of the GT.M
license. If your GT.M license permits redistribution, you may
redistribute this package integrated with your redistributed GT.M.

Simple aggregation or bundling of this software with another for
distribution does not create a derivative work. To make clear the
distinction between this package and another with which it is
aggregated or bundled, it suffices to place the package in a separate
directory or folder when distributing the aggregate or bundle.

Should you receive this package not integrated with a GT.M
distribution, and missing a COPYING file, you may create a file called
COPYING from the GNU Affero General Public License Version 3 or later
(https://www.gnu.org/licenses/agpl.txt) and use this package under the
terms of that license.

--------------------
BACKGROUND
--------------------

[In GT.M replication, an instance has a logically complete M global
variable namespace that it either updates and replicates to other
instances (a source), or to which it applies updates from a source (a
receiver, of which there are two types, BC and SI). Instances for GT.M
peer replication are logically complete M global variable namespaces,
and an instance can be both a source and a receiver, i.e., instances
are peers. Furthermore, while BC and SI replication are implemented by
GT.M, peer replication is implemented using application code.]

Some types of business logic require serialization. For example, as
the logic to execute each financial transaction on an account depends
on the result of the preceding transaction on that account, and
potentially also on related accounts, distributing the processing over
multiple instances increases the time to serialize updates, which in
turn has an impact on transaction throughput: for contemporary
computer systems, update/transaction serialization is typically the
ultimate limit to application scalability. So for application logic
that requires serialization, centralizing serialization to a single
computing node yields the best throughput on current technology.

Other types of business logic either do not require serialization or
do not need serialization to be determined by processing logic. For
example, a financial institution must keep track of balance inquiries,
but while the balance reported by an inquiry depends on the previous
update to that account, it does not depend on the result of the
previous balance inquiry. Consider an application deployment where
financial transactions for accounts are centrally processed on an
instance A (with instances B, and C for business continuity), and
streamed in real-time using GT.M replication to instances P, Q and R
which handle inquiries. Since each inquiry is unique and handled by
one of P, Q, or R, simply aggregating the inquiries processed across
all three inquiry servers provides a complete record of inquiries.

In other words, while there is business logic that requires
serialization and hence is best centralized rather than distributed,
there is other business logic that can be distributed: to improve
application responsiveness by providing local service, for better
ability to handle spikes in load, etc.

Similar dichotomies exist in other application domains - for example,
while orders to be fulfilled from a warehouse must be serialized
(first come, first served; most important customer in line served
first; whatever rules apply) since a warehouse with an inventory of
four widgets cannot ship five, customer order histories are
collections of orders regardless of which warehouse fulfilled each
item, and can be aggregated without requiring serialization by
application software.

GT.M Peer Replication is a design pattern, supported by a working
reference implementation, that uses application level code to
aggregate from multiple instances specified database updates which do
not need serialization. While the unmodified reference implementation
may well satisfy the needs of many applications, complex applications
will likely  need to adapt and refine it to suit their needs.

--------------------
THEORY OF OPERATION
--------------------

The premise of the reference implementation is that application logic
executed on each instance generates certain database updates to be
captured and replicated to peer instances, to satisfy aggregation
needs. The reference implementation expects that global variables to
be replicated to peers are non-conflicting. This means that any global
node updates replicated are either (a) unique to each node, e.g., the
first subscript is a node id, or the timestamp of an inquiry (since
the same inquiry cannot be independently satisfied by two different
nodes at the same time); or (b) the same value if two instances
compute a value for a given node. The latter is the case for the
threeen1glo program used as a sample application for demonstration
purposes packaged with the reference implementation.

The threeen1glo program computes the 3n+1 sequence length for all
integers from 1 through a number read from its input. If a number n
(n>1) is even, halve it to get the next number; if it is odd, the next
number in the sequence is 3n+1. For example, the 3n+1 sequence for 3
is 3, 10, 5, 16, 8, 4, 2, 1 whose length is 8. The as yet unproven
Collatz Conjecture (https://en.wikipedia.org/wiki/Collatz_conjecture)
states that all sequences eventually reach 1. In a sample output of
the program:

GT.M V6.3-000 Linux x86_64 2016-05-12 12:07:50 threeen1glo 100,000 351 8.847738 seconds; 217,211 nodes; 118,803 sets]

the fields are:

  - $ZVERSION of the GT.M implementation - GT.M V6.3-000 Linux x86_64;
  - the current date and time in an international format - 2016-05-12 12:07:50;
  - the name of the program  - threeen1glo;
  - the input number - 100,000;
  - the longest 3n+1 sequence found - 351;
  - elapsed time - 8.847738 seconds;
  - the number of nodes computed (which is greater than the input
    number for all but the most trivial cases) - 217,211; and
  - the number of nodes set locally (no greater than the number of
    nodes computed, because some of the nodes may have values
    replicated from peers) - 118,803.

In the reference implementation of peer replication, GT.M triggers on
database updates capture each update to be replicated and append it as
nodes indexed by a relative timestamp in a sub-tree of the
^%YGTMPeerRepl global variable. "Push" and "pull" replication
processes monitor the end of this sub-tree and when they observe new
updates appended to the sub-tree, they replicate them in chronological
order. Each push or pull process is specific to a pair of nodes, and
uses two global directories, one maps to a "local" instance, and
another that maps to a "remote" instance. As local and remote
instances are mapped by global directories, and as the GT.M
application code of the reference implementation uses only
functionality available on databases on its own computer system as
well as databases accessed via GT.CM, the reference implementation of
GT.M peer replication is agnostic as to whether a remote instance is
on the same computer system as the local node (as is the case for the
demonstration) or is on a remote computer system accessed via GT.CM.

Note that ^%YGTMPeerRepl needs to be be in the same BC/SI replication
instance as the database updates that are being captured.

Push and pull processes are unidirectional, i.e., bidirectional
replication between A and B requires two processes, one to replicate
in each direction. This arrangement allows for finer grain control of
application deployment configurations, e.g., if an application
configuration requires A to replicate to B, but not the other way
around.

The functionality of GT.M peer replication is implemented with a
single file GTMPeerRepl.m.

  - mumps -run GTMPeerRepl --help prints a usage summary, and
  - mumps -run GTMPeerRepl --help=full prints more detailed
    information

Depending on the data to be replicated, bi-directional replication
may have the potential for infinte loops. If the updates at each node
are not distinguished by the node on which application logic
originally computes the update, the update can be replicated to a peer
node, from where a trigger captures the update, and peer replication
replicates it to the origating node, where a trigger captures the
update, and... In the reference implementation, the push and pull
proceses set the intrinsic special variable $ZTWORMHOLE to
"GTMPeerRepl" and the triggers capture updates to be replicated only
if $ZTWORMHOLE is not "GTMPeerRepl".

The reference implementation of GT.M Peer Replication requires the
GT.M POSIX plugin to be installed.

--------------------
SPECIAL NOTE FOR APPLICATIONS USING TRANSACTION PROCESSING
--------------------

GT.M executes an update and its associated trigger within a
transaction. As GTMPeerRepl records each update by appending it to a
subtree of ^%YGTMPeerRepl, during update intensive workloads, the end
of that subtree can become a potential cause of TP restarts. If
application logic does not use transaction processing, the trigger
transaction encompasses only the actual update and the record of the
update, and even when a trigger causes a restarts, the impact on the
application is likely to be minimal since restarts only recompute the
actual database update (from the global variable node to database
blocks).

If application uses transaction processing, the fact that a
transaction is "open" for a longer period of time increases the
probability of TP restarts. Furthermore, when application logic is
executed inside a transaction, the cost of a restart is higher than
when it is not. Two possible remedies use $JOB to ensure that multiple
processes will not set the same node at the same time, and in turn
allow processes to set VIEW "NOISOLATION" for ^%YGTMPeerRepl.

  - Adding $JOB as the last subscript to the nodes of ^%YGTMPeerRepl
    reduces the impact of "collisions". When two processes
    concurrently attempt to update the same block, a common scenario
    when appending to the end of a global variable subtree, GT.M
    recomputes the update (as it does for collisions that occur
    outside transactions) instead of restarting the transaction. This
    adds a small complication to the push and pull processes which,
    with VIEW "NOISOLATION", must deal with the possibility of
    multiple nodes (from different process) with the same relative
    timestamp.

  - Adding $JOB as the first subscript to the nodes of ^%YGTMPeerRepl
    reduces restarts by reducing the probability of multiple processes
    updating the same data block at the same time. However, this
    significantly increases the complexity of the push and pull
    processes, which each pass of which must now include an outer loop
    that scans across processes, and an inner loop that scans for
    updates by a process creating a time ordered list of updates in a
    local variable to be replicated. Note that a process with a higher
    $JOB may have a more recent update visible to a push or pull
    process scan than a process with an earlier update made after its
    updates were examined by the push or pull process. While dealing
    with this is not rocket science, to ensure time ordering, you must
    deal with it. A heuristic to speed up the scans would be to have
    ^%YGTMPeerRepl($JOB) store the relative timestamp of the most
    recent update by a process.

To implement either of the above, modify the reference implementation
in GTMPeerRepl.m to best match your situation. If you modify the
structure of data in ^%YGTMPeerRepl, remember to adapt operations of
GTMPeerRepl such as purge and status.

A third remedy, which is well suited to high-volume production
deployments, is to replicate the data from the system of record on
which application logic computes serialized database updates to other
instances, using GT.M Supplementary Instance (SI) replication. As the
only source of trigger updates is the Update Process, peer replication
from these supplementary instances will not cause transaction
restarts, or indeed have any impact on the system of record.

--------------------
MANIFEST
--------------------

The GTM Peer Replication package consists of the following files:

  - clean.sh - a program to clean up files created by demolocal.sh
  - COPYING - if present, the license under which this software is
    distributed (see LICENSING, above)
  - demolocal.sh - a shell script to demonstrate & test GT.M Peer
    Replication on your system; discussed below
  - gde_in.txt - input to GDE for the demonstration and test script
  - gtmenv - a file sourced by demolocal.sh to attempt to set up GT.M
    environment variables for the demonstration & test
  - GTMPeerReplDemo.m - a driver program for demolocal.sh
  - GTMPeerRepl.m - the reference implementation of GT.M peer
    replication
  - prog_in.txt - input to the program threeen1glo
  - readme.txt - this file
  - threeen1glo.m - code for the "application" discussed above used to
    demonstrate GT.M Peer replication
  - trig.txt - a list of the global variable nodes to be replicated,
    one to a line

--------------------
DEMO / TEST
--------------------

Unpack the GT.M Peer Replication distribution in a directory, and make
the demolocal.sh and clean.sh files executable. Invoke ./demolocal.sh
to run the demonstration, and ./clean.sh to clean up after one or more
runs of demolocal.sh. The following is a run of demolocal.sh annotated
with comments:

$ ./demolocal.sh 
****Demo of GT.M Peer Replication
$gtm_dist=/usr/lib/fis-gtm/V6.3-000_x86_64
$GTMXC_gtmposix=/usr/lib/fis-gtm/V6.3-000_x86_64/plugin/gtmposix.xc
$gtmroutines="/home/gtmuser/GTMPeerReplDemo* /usr/lib/fis-gtm/V6.3-000_x86_64/plugin/o/_POSIX.so /usr/lib/fis-gtm/V6.3-000_x86_64/libgtmutil.so"
$timestamp=2016-05-12_16:59:40 UTC

demolocal.sh sources the gtmenv file in an attempt to find an
installation of GT.M it can use, and sets environment variables for
that installation. Check the values reported, and if they are
incorrect for your system, edit gtmenv to provide correct values. In
order to run demolocal.sh multiple times, the demonstration generates
a timestamp, and creates subdirectories outfiles_$timestamp,
peer{1,2,3}_$timestamp, to preserve the output of each run.

GTMPeerReplDemo 2016-05-12_16:59:40 GTMPeerRepl 0.1

GTMPeerReplDemo identifies itself, the timestamp, and the version of
the reference implementation of GT.M Peer Replication.

MUPIP TRIGGER file to load triggers is /home/gtmuser/GTMPeerReplDemo/outfiles_2016-05-12_16:59:40/GTMPeerReplDemo_2016-05-12_16:59:40_31954_trigload.txt
MUPIP TRIGGER file to delete triggers is /home/gtmuser/GTMPeerReplDemo/outfiles_2016-05-12_16:59:40/GTMPeerReplDemo_2016-05-12_16:59:40_31954_trigdel.txt

From the list of global variables to be replicated in trig.txt, and
the trigger templates at label trigdef in GTMPeerRepl.m, demolocal.sh
creates files for MUPIP TRIGGER to load trigger definitions neeeded
for peer replication and to delete them on completion. You should
review the *trigload.txt file to see the definitions of triggers to
capture updates.

****Checking --help: mumps -run GTMPeerRepl --help
GT.M Peer Replication - n way database peer replication
Usage:
 mumps -run GTMPeerRepl [option] ... where options are:
 --hang=time - when there is no data to be replicated, how many
   milliseconds to wait before checking again; default is 100
   and zero means to keep checking in a tight loop
 --help[=full] - print helpful information and exit without
   processing any following options
 --[loca]lname=instname - name of this instance; instance names
   must start with a letter and are case-sensitive
 --[pull[from]|push[to]]=instname[,gbldirname] - remote instance
   name and access information
 --[purg]e[=time] - purge data recorded before time (measured
   as the number of microseconds since the UNIX epoch); 0
   means all data recorded; a negative value is relative to
   the current time; default is all data already replicated
   to known instances
 --[stat]us[=instname[,...]] - show replication status of named
   instances; default to all instances
 --stop[repl][=instname[,...]] - stop replicating to/from named
   instances; default to all instances
 Options are processed from left to right.

In many cases, you may be able to use GTMPeerRepl as distributed, at
least to get started. mumps -run GTMPeerRepl --help gives you a
convenient short list of the command line options it supports.

****Checking --help=full: $gtm_dist/mumps -run GTMPeerRepl --help=full >/home/gtmuser/GTMPeerReplDemo/outfiles_2016-05-12_16:59:40/help_full.txt

The --help=full command line option generates more voluminous help,
which the demolocal.sh script places in the help_full.txt file.

****Loading triggers - outputs in outtrigload.txt files in each peer.
****Turning on peer replication

This is the beginning of the actual demonstration of peer
replication. The demonstration creates three peers, peer1, peer2 and
peer3, each in its own directory, and in each peer loads the triggers
that capture the database updates to be replicated.

****Run peer instances of application
Peer1 pushes to peer2. Peer3 pulls from peer1, peer2 & peer3 push to each other.

This starts three instances of the threeen1glo "application", peer1,
peer2 and peer3. In this demonstration, the replication between peer2
and peer3 is bidirectional, but the replication from peer1 to peer2
and peer3 is unidirectional (a push from peer1 to peer2, and a pull
from peer1 by peer3).

****Check status - peer2 is only peer3 status, others are for all peers

While the threeen1glo application instances are running, the
demonstration runs the --status option of GTMPeerRepl. Sample results
below.

Localname=peer1 Lastseq=1,904,047,320,770
  Peer=peer2 Sent=1,904,046,483,340 Received=0 Gbldir=/home/gtmuser/GTMPeerReplDemo/peer2_2016-05-12_16:59:40/gtm.gld
    push started pid=31970 at 1,904,046,085,229
  Peer=peer3 Sent=1,904,046,229,617 Received=0

Captured updates are serialized by a relative microsecond time
stamp. All instances must have a common time, and while the offset
from $ZUT can be any value, all instances must agree on that
offset. While the timestamps are nominally integer microseconds,
multiple updates within the same microsecond increment the sequence
number by .01, i.e., the resolution is theoretically ten nanoseconds,
and when future computers exceed a sustained rate of 1E8
updates/second, GTMPeerRepl will need to be modified
appropriately. The status report for peer1 identifies the name of the
instance (peer1), and the last update sequence number captured
(1,904,047,320,770). The last update pushed to peer2 is 1,904,046,483,340
(i.e., the backlog is 837,430 microseconds worth of updates), and
since the replication from peer1 to peer2 is a push, the peer1 status
reports the global directory it uses to access peer2. The last update
pulled from peer1 by peer3 is 1,904,046,229,617. Since peer3 is pulling
data from peer1, peer1 does not have a global directory for peer3. As
replication from peer1 to peer2 and peer3 is unidirectional, peer1
reports that it has received no updates from either.

Localname=peer2 Lastseq=1,904,047,494,105
  Peer=peer3 Sent=1,904,046,533,849 Received=0 Gbldir=/home/gtmuser/GTMPeerReplDemo/peer3_2016-05-12_16:59:40/gtm.gld

As the status command for peer1 and peer3 did not request status for a
specific instance, they report status for all instances. The status
command for peer2 in the demonstration requests status for peer3, for
which it reports the status. Although each of peer2 and peer3 is
pushing to the other (bidirectional peer replication), at the time of
the status command, peer3 has not pushed any data to peer 2.

Localname=peer3 Lastseq=0
  Peer=peer1 Sent=0 Received=1,904,046,229,617 Gbldir=/home/gtmuser/GTMPeerReplDemo/peer1_2016-05-12_16:59:40/gtm.gld
    pull started pid=31982 at 1,904,046,206,899
  Peer=peer2 Sent=0 Received=1,904,046,581,877 Gbldir=/home/gtmuser/GTMPeerReplDemo/peer2_2016-05-12_16:59:40/gtm.gld
    push started pid=31978 at 1,904,046,161,864

The status report for peer3 reports the replication status for both
peer1 and peer2. Lastseq=0 means that peer3 has not computed and
captured any updates locally - as of the time status was run, any data
that the application logic on peer3 sought had already been replicated
from peer1 or peer2. As peer3 is started after peer1 and peer2,
Lastseq=0 is not unreasonable.

****Wait for programs to complete and get results - number of nodes should be the same for all peers
****Number of sets should be same as number of nodes for peer1; and less for other peers
Results
  peer1:
     GT.M V6.3-000 Linux x86_64 2016-05-12 12:59:41 threeen1glo 100,000 351 35.875547 seconds; 217,211 nodes; 217,211 sets]
  peer2:
     GT.M V6.3-000 Linux x86_64 2016-05-12 12:59:41 threeen1glo 100,000 351 36.755706 seconds; 217,211 nodes; 118,967 sets]
  peer3:
     GT.M V6.3-000 Linux x86_64 2016-05-12 12:59:41 threeen1glo 100,000 351 36.620998 seconds; 217,211 nodes; 124,279 sets]

After the instances complete, they report results. The length of the
maximum 3n+1 sequence found must be the same for all instances (351
for an input of 100000), as must the number of nodes
(217,211). However, there is a difference in the number of sets. For
peer1, which receives no replication from another peer, the number of
sets is equal to the number of nodes. But for peer2 and peer3, the
number of sets is less than the number of nodes, as some of the nodes
in their databases have been replicated from other peers.

****Stop replication - outputs in outstop.txt files in each peer, which should all be empty
****Deleting triggers - outputs in outtrigdel.txt files in each peer.

After the instances complete, replication is stopped and the update
capturing triggers are deleted.

****Space in database before purge of updates to be replicated - outputs in outpurge.txt files in each peer, which should all be empty
peer1: Free/Total=1822/5000
peer2: Free/Total=1053/5000
peer3: Free/Total=4542/5000
****Space after purge - all data for peers 1; last 3 seconds for peer2; only already replicated data for peer3
peer1: Free/Total=3960/5000
peer2: Free/Total=2529/5000
peer3: Free/Total=4542/5000

The triggers capture updates in a subtree of ^%YGTMPeerRepl, but never
automatically delete data. When it's safe to purge replicated updates,
the --purge command of GTMPeerRepl can purge captured updates - all
updates, all replicated updates, or all updates prior to a timestamp
(which can be absolute or relative).

****Confirm that there are no remaining processes in any peer
%GTM-I-MUFILRNDWNSUC, File /home/gtmuser/GTMPeerReplDemo/peer1_2016-05-12_16:59:40/gtm.dat successfully rundown
%GTM-I-MUFILRNDWNSUC, File /home/gtmuser/GTMPeerReplDemo/peer2_2016-05-12_16:59:40/gtm.dat successfully rundown
%GTM-I-MUFILRNDWNSUC, File /home/gtmuser/GTMPeerReplDemo/peer3_2016-05-12_16:59:40/gtm.dat successfully rundown
$

This is just a final cleanup to confirm that there are no remaining
processes from the demonstration.

--------------------
INSTALLATION AND USE
--------------------

Review the GTMPeerRepl.m program, and determine whether the as-is
reference implementation meets your needs; modify it as
appropriate. Place it in the execution path for mumps processes as
defined by $gtmroutines / $ZROUTINES. Compile it and locate the object
code per your normal application configuration practices.

Create triggers for global variables to be replicated. Set up a purge
schedule, and deploy instances as needed.
