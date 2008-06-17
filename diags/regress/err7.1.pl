# Name:     err7.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/17/2008
# Purpose:  Verifies that an empty CDD file is handled properly by the report command

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err7.1", 1, @ARGV );

# Create empty CDD file
$OUTPUT = "err7.1.cdd";
system( "touch $OUTPUT; chmod 600 $OUTPUT" ) && die;

# Perform diagnostic running code here
&runReportCommand( "-d v err7.1.cdd 2> err7.1.err" );

# Remove the temporary CDD file
system( "rm -f $OUTPUT" ) && die;

# Perform the file comparison checks
&checkTest( "err7.1", 0, 1 );

exit 0;

