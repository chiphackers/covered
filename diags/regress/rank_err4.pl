# Name:     rank_err4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that a -required-list option without a value outputs the appropriate
#           error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err4", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-required-list 2> rank_err4.err" );
system( "cat rank_err4.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err4", 1, 1 );

exit 0;

