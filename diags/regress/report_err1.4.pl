# Name:     report_err1.4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that when the filename specified for the -o option is
#           unwritable that the appropriate error message is output

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1.4", 1, @ARGV );

# Create report_err1.4.rpt and make it unwritable
system( "touch report_err1.4.rpt; chmod 400 report_err1.4.rpt" ) && die;

# Run the report command
&runReportCommand( "-d v -o report_err1.4.rpt report_err1.4.cdd 2> report_err1.4.err" );
system( "cat report_err1.4.err" ) && die;

# Remove the CDD file
system( "rm -f report_err1.4.rpt" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1.4", 1, 1 );

exit 0;

