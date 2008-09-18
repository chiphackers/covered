# Name:     exclude_err1.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  Verifies that the appropriate error message is output when no exclusion
#           ID is specified on the command-line.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude_err1.1", 1, @ARGV );

# Perform diagnostic running code here
&runExcludeCommand( "exclude_err1.1.cdd 2> exclude_err1.1.err" );
system( "cat exclude_err1.1.err" ) && die;

# Perform the file comparison checks
&checkTest( "exclude_err1.1", 1, 1 );

exit 0;

