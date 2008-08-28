# Name:     report_err1.6.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that a bad report command is reported as such.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1.6", 1, @ARGV );

# Run the report command
&runReportCommand( "-d v -z -o report_err1.6.rpt report_err1.6.cdd 2> report_err1.6.err" );
system( "cat report_err1.6.err" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1.6", 1, 1 );

exit 0;

