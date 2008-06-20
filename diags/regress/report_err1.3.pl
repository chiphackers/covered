# Name:     report_err1.3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that no value for the -o option is output correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1.3", 1, @ARGV );

# Perform diagnostic running code here
&runReportCommand( "-o -d v report_err1.3.cdd 2> report_err1.3.err" );
system( "cat report_err1.3.err" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1.3", 1, 1 );

exit 0;

