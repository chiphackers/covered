# Name:     report7.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/20/2008
# Purpose:  Verifies that the -w option without a value causes the output report to use the width value of 80,
#           ignoring the user-supplied formatting.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report7.1", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP report7.1.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report7.1.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report7.1.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report7.1.v" ) && die;
}

&runScoreCommand( "-t main -vcd report7.1.vcd -v report7.1.v -o report7.1.cdd" );
&runReportCommand( "-d v -w -m ltcfam -o report7.1.rptM report7.1.cdd" );
&runReportCommand( "-d v -w -m ltcfam -i -o report7.1.rptI report7.1.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) { 
  &checkTest( "report7.1", 1, 0 );
} else {
  &checkTest( "report7.1", 1, 5 );
}

exit 0;

