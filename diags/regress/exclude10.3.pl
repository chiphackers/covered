# Name:     exclude10.3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/10/2008
# Purpose:  Runs the exclude command to exclude an expression for coverage, providing a reason for
#           the exclusion

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude10.3", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude10.3.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude10.3.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude10.3.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude10.3.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude10.3.vcd -v exclude10.3.v -o exclude10.3.cdd" );

# Create temporary files that will contain an exclusion message
&runCommand( "echo This logic is not needed > exclude10.3a.excl" );
&runCommand( "echo This logic is also not needed > exclude10.3b.excl" );

# Perform exclusion
&runExcludeCommand( "-m E03 exclude10.3.cdd < exclude10.3a.excl" );
&runExcludeCommand( "-m E13 exclude10.3.cdd < exclude10.3b.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude10.3a.excl exclude10.3b.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude10.3.rptM exclude10.3.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude10.3.rptI exclude10.3.cdd" );

# Perform the file comparison checks
&checkTest( "exclude10.3", 1, 0 );

exit 0;

