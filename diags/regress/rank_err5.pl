# Name:     rank_err5.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that the -required-cdd option without a value outputs an error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err5", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-required-cdd 2> rank_err5.err" );
system( "cat rank_err5.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err5", 1, 1 );

exit 0;

