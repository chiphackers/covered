# Name:     merge_err1.3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge_err1.3", 1, @ARGV );

# Perform diagnostic running code here
&runMergeCommand( "-o 2> merge_err1.3.err" );
system( "cat merge_err1.3.err" ) && die;

# Perform the file comparison checks
&checkTest( "merge_err1.3", 1, 1 );

exit 0;

