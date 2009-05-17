# Name:     exclude14.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that a command-line file (-f option) can be used without error.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude14", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude14.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude14.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude14.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude14.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude14.vcd -v exclude14.v -o exclude14.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This line is not needed > exclude14.excl" );

# Perform exclusion
&runExcludeCommand( "-f ../regress/exclude14.cfg < exclude14.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude14.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude14.rptM exclude14.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude14.rptI exclude14.cdd" );

# Perform the file comparison checks
&checkTest( "exclude14", 1, 0 );

exit 0;

