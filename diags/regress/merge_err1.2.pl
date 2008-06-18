# Name:     merge_err1.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that the correct error message is displayed if only no input CDD files are specified.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge_err1.2", 1, @ARGV );

# Perform diagnostic running code here
&runMergeCommand( "-o merge_err1.2.cdd 2> merge_err1.2.err" );
system( "cat merge_err1.2.err" ) && die;

# Perform the file comparison checks
&checkTest( "merge_err1.2", 1, 1 );

exit 0;

