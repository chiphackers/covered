# Name:     report4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/19/2008
# Purpose:  

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report4", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report4.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report4.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report4.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report4.v" ) && die;
}

&runScoreCommand( "-t main -vcd report4.vcd -v report4.v -o report4.cdd" );
if( $SIMULATOR eq "VERILATOR" ) {
  &runReportCommand( "-d s -m ltcfam -o report4.rptM report4.cdd" );
  &runReportCommand( "-d s -m ltcfam -i -o report4.rptI report4.cdd" );
} else {
  &runReportCommand( "-d s -m ltcefam -o report4.rptM report4.cdd" );
  &runReportCommand( "-d s -m ltcefam -i -o report4.rptI report4.cdd" );
}

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) { 
  &checkTest( "report4", 1, 0 );
} else {
  &checkTest( "report4", 1, 5 );
}

exit 0;

