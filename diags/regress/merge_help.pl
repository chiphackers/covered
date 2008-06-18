# Name:     merge_help.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge_help", 0, @ARGV );

# Perform diagnostic running code here
&runCommand( "$COVERED -Q merge -h > merge_help.err" );

# Perform memory test and collect rest of output
if( &checkForTestMode() ne "" ) {
  system( "cat merge_help.err | ./check_mem > tmp_merge_help.err; mv tmp_merge_help.err merge_help.err" );
}

# Perform the file comparison checks
&checkTest( "merge_help", 0, 1 );

exit 0;

