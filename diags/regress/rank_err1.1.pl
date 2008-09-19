# Name:     rank_err1.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that an unwritable -o option value causes an error message to be output.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err1.1", 1, @ARGV );

# Perform diagnostic running code here
system( "touch rank_err1.1.cdd; chmod 400 rank_err1.1.cdd" ) && die;
&runRankCommand( "-o rank_err1.1.cdd 2> rank_err1.1.err" );
system( "cat rank_err1.1.err" ) && die;

# Remove rank_err1.1.cdd
system( "chmod 600 rank_err1.1.cdd; rm -f rank_err1.1.cdd" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err1.1", 1, 1 );

exit 0;

