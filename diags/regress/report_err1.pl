# Name:     report_err1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that the -m option needs a value.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1", 1, @ARGV );

# Perform diagnostic running code here
&runReportCommand( "-m 2> report_err1.err" );
system( "cat report_err1.err" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1", 1, 1 );

exit 0;

