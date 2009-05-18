# Name:     report3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that the -c option works properly for line coverage.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report3", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report3.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report3.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report3.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report3.v" ) && die;
}

&runScoreCommand( "-t main -vcd report3.vcd -v report3.v -o report3.cdd" );
&runReportCommand( "-d v -c -o report3.rptM report3.cdd" );
&runReportCommand( "-d v -i -c -o report3.rptI report3.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) {
  &checkTest( "report3", 1, 0 );
} else {
  &checkTest( "report3", 1, 5 );
}

exit 0;

