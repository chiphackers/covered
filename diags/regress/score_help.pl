# Name:     score_help.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that the score help information is displayed correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "score_help", 0, @ARGV );

# Perform diagnostic running code here
&runCommand( "$COVERED -Q score -h > score_help.err" );

# Perform memory test and collect rest of output
if( &checkForTestMode() ne "" ) {
  system( "cat score_help.err | ./check_mem > tmp_score_help.err; mv tmp_score_help.err score_help.err" );
}

# Perform the file comparison checks
&checkTest( "score_help", 0, 1 );

exit 0;

