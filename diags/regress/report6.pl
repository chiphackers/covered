# Name:     report6.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/20/2008
# Purpose:  Verifies that if two -o options are specified on the command-line, only
#           first value is used.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report6", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report6.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report6.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report6.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report6.v" ) && die;
}

&runScoreCommand( "-t main -vcd report6.vcd -v report6.v -o report6.cdd" );
&runReportCommand( "-d v -m ltcfam -o report6.rptM -o report6.dummyM report6.cdd" );
&runReportCommand( "-d v -m ltcfam -i -o report6.rptI -o report6.dummyI report6.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) { 
  &checkTest( "report6", 1, 0 );
} else {
  &checkTest( "report6", 1, 5 );
}

exit 0;

