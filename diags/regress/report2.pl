# Name:     report2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verify that illegal -m values are displayed as warnings.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report2", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP report2.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report2.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report2.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report2.v" ) && die;
}

&runScoreCommand( "-t main -vcd report2.vcd -v report2.v -o report2.cdd" );
&runReportCommand( "-d v -m lxtyczf -o report2.rptM report2.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) {
  &checkTest( "report2", 1, 0 );
} else {
  &checkTest( "report2", 1, 5 );
}

exit 0;

