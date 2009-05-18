# Name:     report4.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that the -d d option works correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report4.2", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report4.2.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report4.2.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report4.2.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report4.2.v" ) && die;
}

&runScoreCommand( "-t main -vcd report4.2.vcd -v report4.2.v -o report4.2.cdd" );
&runReportCommand( "-d d -m ltcfam -o report4.2.rptM report4.2.cdd" );
&runReportCommand( "-d d -m ltcfam -i -o report4.2.rptI report4.2.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) { 
  &checkTest( "report4.2", 1, 0 );
} else {
  &checkTest( "report4.2", 1, 5 );
}

exit 0;

