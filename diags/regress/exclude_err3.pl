# Name:     exclude_err3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that if -f option is specified without a following value, the appropriate
#           error is output.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude_err3", 1, @ARGV );

# Perform diagnostic running code here
&runExcludeCommand( "-f 2> exclude_err3.err" );
system( "cat exclude_err3.err" ) && die;

# Perform the file comparison checks
&checkTest( "exclude_err3", 1, 1 );

exit 0;

