# Name:     score_err1.3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/23/2008
# Purpose:  Cause command-file to be not readable and verify that the appropriate
#           error message is displayed to the user.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "score_err1.3", 1, @ARGV );

# Create a command-file and make its permissions write-only
system( "touch score_err1.3.cfg; chmod 000 score_err1.3.cfg" ) && die;

# Perform diagnostic running code here
&runScoreCommand( "-f score_err1.3.cfg 2> score_err1.3.err" );
system( "cat score_err1.3.err" ) && die;

# Remove the score_err1.3 command file
system( "chmod 600 score_err1.3.cfg; rm -rf score_err1.3.cfg" ) && die;

# Perform the file comparison checks
&checkTest( "score_err1.3", 1, 1 );

exit 0;

