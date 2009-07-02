# Name:     merge11.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     11/14/2008
# Purpose:  Verifies that each instance in the design can be merged in any order to make the same
#           base CDD file. 

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge11", 0, @ARGV );

if( $DUMPTYPE eq "VCD" ) {
  $check_type = 0;
} else {
  $check_type = 5;
}

# Perform diagnostic running code here
&runCommand( "$MAKE DIAG=merge11a onemergerun" );
&runCommand( "$MAKE DIAG=merge11b onemergerun" );
&runCommand( "$MAKE DIAG=merge11c onemergerun" );
&runCommand( "$MAKE DIAG=merge11d onemergerun" );
&runCommand( "$MAKE DIAG=merge11e onemergerun" );
&runCommand( "$MAKE DIAG=merge11f onemergerun" );
&runCommand( "$MAKE DIAG=merge11g onemergerun" );
&runCommand( "$MAKE DIAG=merge11h onemergerun" );

# Perform all combinations of merges
&runMergeCommand( "-o merge11.1a.cdd merge11a.cdd merge11b.cdd" );
&runMergeCommand( "-o merge11.1b.cdd merge11c.cdd merge11d.cdd" );
&runMergeCommand( "-o merge11.1c.cdd merge11e.cdd merge11f.cdd" );
&runMergeCommand( "-o merge11.1d.cdd merge11g.cdd merge11h.cdd" );
&runMergeCommand( "-o merge11.1.cdd merge11.1a.cdd merge11.1b.cdd merge11.1c.cdd merge11.1d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.1.rptM merge11.1.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.1.rptI merge11.1.cdd" );
&checkTest( "merge11.1", 1, $check_type );
system( "rm -f merge11.1a.cdd merge11.1b.cdd merge11.1c.cdd merge11.1d.cdd" );

&runMergeCommand( "-o merge11.2a.cdd merge11b.cdd merge11e.cdd" );
&runMergeCommand( "-o merge11.2b.cdd merge11a.cdd merge11f.cdd" );
&runMergeCommand( "-o merge11.2c.cdd merge11c.cdd merge11h.cdd" );
&runMergeCommand( "-o merge11.2d.cdd merge11d.cdd merge11g.cdd" );
&runMergeCommand( "-o merge11.2.cdd merge11.2a.cdd merge11.2b.cdd merge11.2c.cdd merge11.2d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.2.rptM merge11.2.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.2.rptI merge11.2.cdd" );
&checkTest( "merge11.2", 1, $check_type );
system( "rm -f merge11.2a.cdd merge11.2b.cdd merge11.2c.cdd merge11.2d.cdd" );

&runMergeCommand( "-o merge11.3a.cdd merge11h.cdd merge11g.cdd" );
&runMergeCommand( "-o merge11.3b.cdd merge11f.cdd merge11e.cdd" );
&runMergeCommand( "-o merge11.3c.cdd merge11d.cdd merge11c.cdd" );
&runMergeCommand( "-o merge11.3d.cdd merge11b.cdd merge11a.cdd" );
&runMergeCommand( "-o merge11.3.cdd merge11.3a.cdd merge11.3b.cdd merge11.3c.cdd merge11.3d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.3.rptM merge11.3.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.3.rptI merge11.3.cdd" );
&checkTest( "merge11.3", 1, $check_type );
system( "rm -f merge11.3a.cdd merge11.3b.cdd merge11.3c.cdd merge11.3d.cdd" );

&runMergeCommand( "-o merge11.4a.cdd merge11g.cdd merge11d.cdd" );
&runMergeCommand( "-o merge11.4b.cdd merge11h.cdd merge11c.cdd" );
&runMergeCommand( "-o merge11.4c.cdd merge11f.cdd merge11a.cdd" );
&runMergeCommand( "-o merge11.4d.cdd merge11e.cdd merge11b.cdd" );
&runMergeCommand( "-o merge11.4.cdd merge11.4a.cdd merge11.4b.cdd merge11.4c.cdd merge11.4d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.4.rptM merge11.4.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.4.rptI merge11.4.cdd" );
&checkTest( "merge11.4", 1, $check_type );
system( "rm -f merge11.4a.cdd merge11.4b.cdd merge11.4c.cdd merge11.4d.cdd" );

&runMergeCommand( "-o merge11.5a.cdd merge11f.cdd merge11h.cdd" );
&runMergeCommand( "-o merge11.5b.cdd merge11e.cdd merge11g.cdd" );
&runMergeCommand( "-o merge11.5c.cdd merge11b.cdd merge11d.cdd" );
&runMergeCommand( "-o merge11.5d.cdd merge11a.cdd merge11c.cdd" );
&runMergeCommand( "-o merge11.5.cdd merge11.5a.cdd merge11.5b.cdd merge11.5c.cdd merge11.5d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.5.rptM merge11.5.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.5.rptI merge11.5.cdd" );
&checkTest( "merge11.5", 1, $check_type );
system( "rm -f merge11.5a.cdd merge11.5b.cdd merge11.5c.cdd merge11.5d.cdd" );

&runMergeCommand( "-o merge11.6a.cdd merge11e.cdd merge11c.cdd" );
&runMergeCommand( "-o merge11.6b.cdd merge11g.cdd merge11a.cdd" );
&runMergeCommand( "-o merge11.6c.cdd merge11h.cdd merge11b.cdd" );
&runMergeCommand( "-o merge11.6d.cdd merge11f.cdd merge11d.cdd" );
&runMergeCommand( "-o merge11.6.cdd merge11.6a.cdd merge11.6b.cdd merge11.6c.cdd merge11.6d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.6.rptM merge11.6.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.6.rptI merge11.6.cdd" );
&checkTest( "merge11.6", 1, $check_type );
system( "rm -f merge11.6a.cdd merge11.6b.cdd merge11.6c.cdd merge11.6d.cdd" );

&runMergeCommand( "-o merge11.7a.cdd merge11d.cdd merge11f.cdd" );
&runMergeCommand( "-o merge11.7b.cdd merge11b.cdd merge11h.cdd" );
&runMergeCommand( "-o merge11.7c.cdd merge11a.cdd merge11g.cdd" );
&runMergeCommand( "-o merge11.7d.cdd merge11c.cdd merge11e.cdd" );
&runMergeCommand( "-o merge11.7.cdd merge11.7a.cdd merge11.7b.cdd merge11.7c.cdd merge11.7d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.7.rptM merge11.7.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.7.rptI merge11.7.cdd" );
&checkTest( "merge11.7", 1, $check_type );
system( "rm -f merge11.7a.cdd merge11.7b.cdd merge11.7c.cdd merge11.7d.cdd" );

&runMergeCommand( "-o merge11.8a.cdd merge11c.cdd merge11a.cdd" );
&runMergeCommand( "-o merge11.8b.cdd merge11d.cdd merge11b.cdd" );
&runMergeCommand( "-o merge11.8c.cdd merge11g.cdd merge11e.cdd" );
&runMergeCommand( "-o merge11.8d.cdd merge11h.cdd merge11f.cdd" );
&runMergeCommand( "-o merge11.8.cdd merge11.8a.cdd merge11.8b.cdd merge11.8c.cdd merge11.8d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge11.8.rptM merge11.8.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge11.8.rptI merge11.8.cdd" );
&checkTest( "merge11.8", 1, $check_type );
system( "rm -f merge11.8a.cdd merge11.8b.cdd merge11.8c.cdd merge11.8d.cdd" );

# Remove intermediate CDD files
&checkTest( "merge11a", 1, 6 );
&checkTest( "merge11b", 1, 6 );
&checkTest( "merge11c", 1, 6 );
&checkTest( "merge11d", 1, 6 );
&checkTest( "merge11e", 1, 6 );
&checkTest( "merge11f", 1, 6 );
&checkTest( "merge11g", 1, 6 );
&checkTest( "merge11h", 1, 6 );

exit 0;

