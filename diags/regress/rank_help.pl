# Name:     rank_help.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that the -h option to the rank command outputs the correct output.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_help", 0, @ARGV );

# Perform diagnostic running code here
&runCommand( "$COVERED -Q rank -h > rank_help.err" );

# Perform memory test and collect rest of output
if( &checkForTestMode() ne "" ) {
  system( "cat rank_help.err | ./check_mem > tmp_rank_help.err; mv tmp_rank_help.err rank_help.err" );
}

# Perform the file comparison checks
&checkTest( "rank_help", 0, 1 );

exit 0;

