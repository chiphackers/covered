# Name:     merge10.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     11/14/2008
# Purpose:  Verifies that each instance in the design can be merged in any order to make the same
#           base CDD file. 

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge10", 0, @ARGV );

if( $DUMPTYPE eq "VCD" ) {
  $check_type = 0;
} else {
  $check_type = 5;
}

# Perform diagnostic running code here
&runCommand( "$MAKE DIAG=merge10a onemergerun" );
&runCommand( "$MAKE DIAG=merge10b onemergerun" );
&runCommand( "$MAKE DIAG=merge10c onemergerun" );
&runCommand( "$MAKE DIAG=merge10d onemergerun" );
&runCommand( "$MAKE DIAG=merge10e onemergerun" );
&runCommand( "$MAKE DIAG=merge10f onemergerun" );
&runCommand( "$MAKE DIAG=merge10g onemergerun" );
&runCommand( "$MAKE DIAG=merge10h onemergerun" );

# Perform all combinations of merges
&runMergeCommand( "-o merge10.1.cdd merge10a.cdd merge10b.cdd merge10c.cdd merge10d.cdd merge10e.cdd merge10f.cdd merge10g.cdd merge10h.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.1.rptM merge10.1.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.1.rptI merge10.1.cdd" );
&checkTest( "merge10.1", 1, $check_type );

&runMergeCommand( "-o merge10.2.cdd merge10b.cdd merge10e.cdd merge10a.cdd merge10f.cdd merge10c.cdd merge10h.cdd merge10d.cdd merge10g.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.2.rptM merge10.2.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.2.rptI merge10.2.cdd" );
&checkTest( "merge10.2", 1, $check_type );

&runMergeCommand( "-o merge10.3.cdd merge10h.cdd merge10g.cdd merge10f.cdd merge10e.cdd merge10d.cdd merge10c.cdd merge10b.cdd merge10a.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.3.rptM merge10.3.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.3.rptI merge10.3.cdd" );
&checkTest( "merge10.3", 1, $check_type );

&runMergeCommand( "-o merge10.4.cdd merge10g.cdd merge10d.cdd merge10h.cdd merge10c.cdd merge10f.cdd merge10a.cdd merge10e.cdd merge10b.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.4.rptM merge10.4.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.4.rptI merge10.4.cdd" );
&checkTest( "merge10.4", 1, $check_type );

&runMergeCommand( "-o merge10.5.cdd merge10f.cdd merge10h.cdd merge10e.cdd merge10g.cdd merge10b.cdd merge10d.cdd merge10a.cdd merge10c.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.5.rptM merge10.5.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.5.rptI merge10.5.cdd" );
&checkTest( "merge10.5", 1, $check_type );

&runMergeCommand( "-o merge10.6.cdd merge10e.cdd merge10c.cdd merge10g.cdd merge10a.cdd merge10h.cdd merge10b.cdd merge10f.cdd merge10d.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.6.rptM merge10.6.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.6.rptI merge10.6.cdd" );
&checkTest( "merge10.6", 1, $check_type );

&runMergeCommand( "-o merge10.7.cdd merge10d.cdd merge10f.cdd merge10b.cdd merge10h.cdd merge10a.cdd merge10g.cdd merge10c.cdd merge10e.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.7.rptM merge10.7.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.7.rptI merge10.7.cdd" );
&checkTest( "merge10.7", 1, $check_type );

&runMergeCommand( "-o merge10.8.cdd merge10c.cdd merge10a.cdd merge10d.cdd merge10b.cdd merge10g.cdd merge10e.cdd merge10h.cdd merge10f.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.8.rptM merge10.8.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.8.rptI merge10.8.cdd" );
&checkTest( "merge10.8", 1, $check_type );

# Remove intermediate CDD files
&checkTest( "merge10a", 1, 6 );
&checkTest( "merge10b", 1, 6 );
&checkTest( "merge10c", 1, 6 );
&checkTest( "merge10d", 1, 6 );
&checkTest( "merge10e", 1, 6 );
&checkTest( "merge10f", 1, 6 );
&checkTest( "merge10g", 1, 6 );
&checkTest( "merge10h", 1, 6 );

exit 0;

