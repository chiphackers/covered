# Name:     exclude_help.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  Verifies that exclude command usage information is output properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude_help", 0, @ARGV );

# Perform diagnostic running code here
&runCommand( "$COVERED -Q exclude -h > exclude_help.err" );

# Perform memory test and collect rest of output
if( &checkForTestMode() ne "" ) {
  system( "cat exclude_help.err | ./check_mem > tmp_exclude_help.err; mv tmp_exclude_help.err exclude_help.err" );
}

# Perform the file comparison checks
&checkTest( "exclude_help", 0, 1 );

exit 0;

