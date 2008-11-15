# Name:     merge10.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     11/14/2008
# Purpose:  Verifies that each instance in the design can be merged in any order to make the same
#           base CDD file. 

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge10", 0, @ARGV );

# Perform diagnostic running code here
&runCommand( "make DIAG=merge10a onemergerun" );
&runCommand( "make DIAG=merge10b onemergerun" );
&runCommand( "make DIAG=merge10c onemergerun" );
&runCommand( "make DIAG=merge10d onemergerun" );
&runCommand( "make DIAG=merge10e onemergerun" );
&runCommand( "make DIAG=merge10f onemergerun" );
&runCommand( "make DIAG=merge10g onemergerun" );
&runCommand( "make DIAG=merge10h onemergerun" );

# Perform all combinations of merges
&runMergeCommand( "-o merge10.cdd merge10a.cdd merge10b.cdd merge10c.cdd merge10d.cdd merge10e.cdd merge10f.cdd merge10g.cdd merge10h.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10b.cdd merge10a.cdd merge10d.cdd merge10c.cdd merge10f.cdd merge10e.cdd merge10h.cdd merge10g.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10h.cdd merge10f.cdd merge10a.cdd merge10g.cdd merge10d.cdd merge10b.cdd merge10e.cdd merge10c.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10g.cdd merge10c.cdd merge10e.cdd merge10a.cdd merge10h.cdd merge10d.cdd merge10f.cdd merge10b.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10f.cdd merge10d.cdd merge10g.cdd merge10e.cdd merge10b.cdd merge10h.cdd merge10c.cdd merge10a.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10e.cdd merge10g.cdd merge10f.cdd merge10h.cdd merge10a.cdd merge10c.cdd merge10b.cdd merge10d.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10d.cdd merge10h.cdd merge10b.cdd merge10f.cdd merge10c.cdd merge10g.cdd merge10a.cdd merge10e.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10c.cdd merge10h.cdd merge10e.cdd merge10d.cdd merge10a.cdd merge10f.cdd merge10b.cdd merge10g.cdd" );
&checkTest( "merge10", 1, 0 );

&runMergeCommand( "-o merge10.cdd merge10g.cdd merge10b.cdd merge10a.cdd merge10c.cdd merge10d.cdd merge10e.cdd merge10f.cdd merge10h.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -o merge10.rptM merge10.cdd" );
&runReportCommand( "-d v -e -m ltcfamr -i -o merge10.rptI merge10.cdd" );
&checkTest( "merge10", 1, 0 );

exit 0;

