# Name:     rank_err5.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that if a -required-cdd option is specified with an invalid filename
#           an appropriate output message is given.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err5.1", 1, @ARGV );

# Perform diagnostic running code here
&runRankCommand( "-required-cdd foobar 2> rank_err5.1.err" );
system( "cat rank_err5.1.err" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err5.1", 1, 1 );

exit 0;

