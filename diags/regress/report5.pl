# Name:     report5.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/20/2008
# Purpose:  

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report5", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report5.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report5.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report5.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report5.v" ) && die;
}

&runScoreCommand( "-t main -vcd report5.vcd -v report5.v -o report5.cdd" );
&runReportCommand( "-d s -m ltcfam -s -o report5.rptM report5.cdd" );
&runReportCommand( "-d s -m ltcfam -s -i -o report5.rptI report5.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) { 
  &checkTest( "report5", 1, 0 );
} else {
  &checkTest( "report5", 1, 5 );
}

exit 0;

