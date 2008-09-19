# Name:     rank_err8.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that the -weight-toggle option that doesn't have an associated value
#           outputs an appropriate error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err8", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-weight-toggle 2> rank_err8.err" );
system( "cat rank_err8.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err8", 1, 1 );

exit 0;

