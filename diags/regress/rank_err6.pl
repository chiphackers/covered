# Name:     rank_err6.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that an -ext option that doesn't have a value outputs an appropriate
#           error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err6", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-ext 2> rank_err6.err" );
system( "cat rank_err6.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err6", 1, 1 );

exit 0;

