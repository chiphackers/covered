# Name:     rank_err4.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that a -required-list option with an invalid filename outputs an appropriate
#           error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err4.1", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-required-list foobar 2> rank_err4.1.err" );
system( "cat rank_err4.1.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err4.1", 1, 1 );

exit 0;

