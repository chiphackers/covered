# Name:     rank_err2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that a -f option without a valid value outputs the appropriate
#           error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err2", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-f 2> rank_err2.err" );
system( "cat rank_err2.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err2", 1, 1 );

exit 0;

