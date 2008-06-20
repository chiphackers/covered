# Name:     report_err1.8.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that when Covered is not built with Tcl/Tk support and the
#           user specified the -view option that an appropriate error message is
#           reported to the user.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_err1.8", 1, @ARGV );

# Check to see if Tcl/Tk is enabled
open( CFGH, "../../config.h" ) || die "Can't open ../../config.h for reading: $!\n";
while( <CFGH> ) {
  chomp;
  # Don't continue with the test of Tcl/Tk was issued
  if( /define HAVE_TCLTK/ ) {
    print "Skipping report_err1.8 because Tcl/Tk support is compiled in\n";
    exit 0;
  }
}
close( CFGH );

# Run the report command
&runReportCommand( "-view 2> report_err1.8.err" );
system( "cat report_err1.8.err" ) && die;

# Perform the file comparison checks
&checkTest( "report_err1.8", 1, 1 );

exit 0;

