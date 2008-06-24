# Name:     score_err1.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/23/2008
# Purpose:  Verifies that a non-existent command file is output to the user appropriately.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "score_err1.2", 1, @ARGV );

# Perform diagnostic running code here
&runScoreCommand( "-f bubba.cfg 2> score_err1.2.err" );
system( "cat score_err1.2.err" ) && die;

# Perform the file comparison checks
&checkTest( "score_err1.2", 1, 1 );

exit 0;

