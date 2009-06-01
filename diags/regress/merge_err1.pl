# Name:     merge_err1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/17/2008
# Purpose:  Creates a case where an output merged CDD file cannot be written (file exists and is read-only).

require "../verilog/regress_subs.pl";

# This should always be called first
&initialize( "merge_err1", 1, @ARGV );

$OUTPUT = "merge_err1.output";

# Create a temporary file with read-only permissions
system( "touch $OUTPUT; chmod 400 $OUTPUT" ) && die;

system( "$MAKE DIAG=merge_err1a TOP=${TOPDIR}merge_err1a.v diagrun" ) && die;
system( "$MAKE DIAG=merge_err1b TOP=${TOPDIR}merge_err1b.v diagrun" ) && die;
&runMergeCommand( "-o $OUTPUT merge_err1a.cdd merge_err1b.cdd 2> merge_err1.err" );
system( "cat merge_err1.err" ) && die;

# Perform diagnostic check
&checkTest( "merge_err1", 1, 1 );

# Remove temporary file
system( "rm -f $OUTPUT" ) && die;

exit 0;
