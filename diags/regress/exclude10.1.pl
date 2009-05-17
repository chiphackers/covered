# Name:     exclude10.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/10/2008
# Purpose:  Runs the exclude command to exclude a toggle for coverage, providing a reason for
#           the exclusion

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude10.1", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude10.1.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude10.1.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude10.1.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude10.1.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude10.1.vcd -v exclude10.1.v -o exclude10.1.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This toggle is not needed > exclude10.1.excl" );

# Perform exclusion
&runExcludeCommand( "-m T01 exclude10.1.cdd < exclude10.1.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude10.1.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude10.1.rptM exclude10.1.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude10.1.rptI exclude10.1.cdd" );

# Perform the file comparison checks
&checkTest( "exclude10.1", 1, 0 );

exit 0;

