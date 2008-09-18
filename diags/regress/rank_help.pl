# Name:     rank_help.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that the -h option to the rank command outputs the correct output.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_help", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-h 2> rank_help.err" );
system( "cat rank_help.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_help", 1, 1 );

exit 0;

