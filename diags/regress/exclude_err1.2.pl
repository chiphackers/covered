# Name:     exclude_err1.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  Verifies that the appropriate error message is output when the specified CDD file
#           does not exist.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude_err1.2", 1, @ARGV );

# Perform diagnostic running code here
&runExcludeCommand( "L01 exclude_err1.2.cdd 2> exclude_err1.2.err" );
system( "cat exclude_err1.2.err" ) && die;

# Perform the file comparison checks
&checkTest( "exclude_err1.2", 1, 1 );

exit 0;

