# Name:     report_help.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that the report help information is displayed correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report_help", 0, @ARGV );

# Check to see if Tcl/Tk is enabled
$have_tcltk = 0;
open( CFGH, "../../config.h" ) || die "Can't open ../../config.h for reading: $!\n";
while( <CFGH> ) {
  chomp;
  # Don't continue with the test of Tcl/Tk was issued
  if( /undef HAVE_TCLTK/ ) {
    print "Skipping report_help because Tcl/Tk support is not compiled in\n";
    exit 0;
  }
}
close( CFGH );

# Perform diagnostic running code here
&runCommand( "$COVERED -Q report -h > report_help.err" );

# Perform memory test and collect rest of output
if( &checkForTestMode() ne "" ) {
  system( "cat report_help.err | ./check_mem > tmp_report_help.err; mv tmp_report_help.err report_help.err" );
}

# Perform the file comparison checks
&checkTest( "report_help", 0, 1 );

exit 0;

