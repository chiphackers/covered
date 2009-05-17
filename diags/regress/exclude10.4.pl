# Name:     exclude10.4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/10/2008
# Purpose:  Runs the exclude command to exclude an FSM state transition for coverage, providing a reason for
#           the exclusion

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude10.4", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP -y lib exclude10.4.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP +libext+.v+.vlib+ -y lib exclude10.4.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP +libext+.v+.vlib+ -y lib exclude10.4.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP +libext+.v+.vlib -y lib exclude10.4.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude10.4.vcd -v exclude10.4.v -y lib -o exclude10.4.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This state transition is not needed > exclude10.4.excl" );

# Perform exclusion
&runExcludeCommand( "-m F002 exclude10.4.cdd < exclude10.4.excl" );
&runExcludeCommand( "-m F004 exclude10.4.cdd < exclude10.4.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude10.4.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude10.4.rptM exclude10.4.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude10.4.rptI exclude10.4.cdd" );

# Perform the file comparison checks
&checkTest( "exclude10.4", 1, 0 );

exit 0;

