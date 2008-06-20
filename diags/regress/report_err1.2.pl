# Name:     report_err1.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that no value for the -d option is output correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1.2", 1, @ARGV );

# Perform diagnostic running code here
&runReportCommand( "-d -o report_err1.2.rpt report_err1.2.cdd 2> report_err1.2.err" );
system( "cat report_err1.2.err" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1.2", 1, 1 );

exit 0;

