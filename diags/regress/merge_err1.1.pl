# Name:     merge_err1.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that Covered displays error message properly when CDD file does not
#           exist.  Also verifies that all memory is cleaned up appropriately. 

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge_err1.1", 1, @ARGV );

# Perform diagnostic running code here
&runMergeCommand( "-o merge_err1.1.cdd merge_err1.1a.cdd merge_err1.1b.cdd 2> merge_err1.1.err" );
system( "cat merge_err1.1.err" ) && die;

# Perform the file comparison checks
&checkTest( "merge_err1.1", 1, 1 );

exit 0;

