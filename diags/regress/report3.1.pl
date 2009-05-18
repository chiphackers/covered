# Name:     report3.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that the -c option works properly for line coverage.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report3.1", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report3.1.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report3.1.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report3.1.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report3.1.v" ) && die;
}

&runScoreCommand( "-t main -vcd report3.1.vcd -v report3.1.v -o report3.1.cdd" );
&runReportCommand( "-d v -c -o report3.1.rptM report3.1.cdd" );
&runReportCommand( "-d v -i -c -o report3.1.rptI report3.1.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) {
  &checkTest( "report3.1", 1, 0 );
} else {
  &checkTest( "report3.1", 1, 5 );
}

exit 0;

