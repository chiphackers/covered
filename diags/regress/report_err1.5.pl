# Name:     report_err1.5.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that a non-existent CDD file outputs the appropriate error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1.5", 1, @ARGV );

# Run the report command
&runReportCommand( "-d v -o report_err1.5.rpt report_err1.5.cdd 2> report_err1.5.err" );
system( "cat report_err1.5.err" ) && die;

# Remove report file
system( "rm -f report_err1.5.rpt" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1.5", 1, 1 );

exit 0;

