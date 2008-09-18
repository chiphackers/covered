# Name:     exclude_err3.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that if the -f option is used and an illegal option is specified, the error is handled
#           correctly in terms of memory use.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude_err3.1", 1, @ARGV );

# Perform diagnostic running code here
&runExcludeCommand( "-f ../regress/exclude_err3.1.cfg 2>exclude_err3.1.err" );
system( "cat exclude_err3.1.err" ) && die;

# Perform the file comparison checks
&checkTest( "exclude_err3.1", 1, 1 );

exit 0;

