# Name:     exclude10.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/10/2008
# Purpose:  Runs the exclude command to exclude a line for coverage, providing a reason for
#           the exclusion

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude10", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude10.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude10.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude10.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude10.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude10.vcd -v exclude10.v -o exclude10.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This line is not needed > exclude10.excl" );

# Perform exclusion
&runExcludeCommand( "-m L10 exclude10.cdd < exclude10.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude10.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude10.rptM exclude10.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude10.rptI exclude10.cdd" );

# Perform the file comparison checks
&checkTest( "exclude10", 1, 0 );

exit 0;

