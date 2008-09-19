# Name:     rank_err3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that an illegal rank option causes the appropriate error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err3", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-z 2> rank_err3.err" );
system( "cat rank_err3.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err3", 1, 1 );

exit 0;

