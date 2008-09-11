# Name:     report_err1.7.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that when a CDD file is not specified on the command-line, the appropriate
#           error message is emitted.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1.7", 1, @ARGV );

# Run the report command
&runReportCommand( "-d v -o report_err1.7.rpt 2> report_err1.7.err" );
system( "cat report_err1.7.err" ) && die;

# Remove report file
system( "rm -f report_err1.7.rpt" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1.7", 1, 1 );

exit 0;

