# Name:     rank_err4.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that a write-only file to the -required-list option outputs the appropriate
#           error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err4.2", 1, @ARGV );

# Create a write-only file
system( "touch rank_err4.2.in; chmod 200 rank_err4.2.in" ) && die;

# Perform diagnostic running code here
&runRankCommand( "-required-list rank_err4.2.in 2> rank_err4.2.err" );
system( "cat rank_err4.2.err" ) && die;

# Remove temporary file
system( "rm -f rank_err4.2.in" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err4.2", 1, 1 );

exit 0;

