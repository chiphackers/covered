# Name:     rank_err9.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that the -weight-memory option that doesn't have an associated value
#           outputs an appropriate error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err9", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-weight-memory 2> rank_err9.err" );
system( "cat rank_err9.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err9", 1, 1 );

exit 0;

