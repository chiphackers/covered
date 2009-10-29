# Name:     report4.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  Verifies that the -d d option works correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report4.1", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report4.1.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report4.1.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report4.1.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report4.1.v" ) && die;
}

&runScoreCommand( "-t main -vcd report4.1.vcd -v report4.1.v -o report4.1.cdd" );
if( $SIMULATOR eq "VERILATOR" ) {
  &runReportCommand( "-d d -m ltcfam -o report4.1.rptM report4.1.cdd" );
  &runReportCommand( "-d d -m ltcfam -i -o report4.1.rptI report4.1.cdd" );
} else {
  &runReportCommand( "-d d -m ltcefam -o report4.1.rptM report4.1.cdd" );
  &runReportCommand( "-d d -m ltcefam -i -o report4.1.rptI report4.1.cdd" );
}

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) { 
  &checkTest( "report4.1", 1, 0 );
} else {
  &checkTest( "report4.1", 1, 5 );
}

exit 0;

