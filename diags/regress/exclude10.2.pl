# Name:     exclude10.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/10/2008
# Purpose:  Runs the exclude command to exclude a memory for coverage, providing a reason for
#           the exclusion

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude10.2", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude10.2.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude10.2.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude10.2.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude10.2.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude10.2.vcd -v exclude10.2.v -o exclude10.2.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This memory is not needed > exclude10.2.excl" );

# Perform exclusion
&runExcludeCommand( "-m M01 exclude10.2.cdd < exclude10.2.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude10.2.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -m m -e -x -o exclude10.2.rptM exclude10.2.cdd" );
&runReportCommand( "-d v -m m -e -x -i -o exclude10.2.rptI exclude10.2.cdd" );

# Perform the file comparison checks
&checkTest( "exclude10.2", 1, 0 );

exit 0;

