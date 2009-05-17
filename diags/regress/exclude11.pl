# Name:     exclude11.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude11", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude11.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude11.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude11.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude11.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude11.vcd -v exclude11.v -o exclude11.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This logic is not needed > exclude11a.excl" );
&runCommand( "echo This logic is also not needed > exclude11b.excl" );
&runCommand( "echo This logic is additionally not needed > exclude11c.excl" );

# Perform exclusion
&runExcludeCommand( "-m L07 exclude11.cdd < exclude11a.excl" );
&runExcludeCommand( "-m L10 exclude11.cdd < exclude11b.excl" );
&runExcludeCommand( "-m L13 exclude11.cdd < exclude11c.excl" );
&runExcludeCommand( "L07 exclude11.cdd" );
&runExcludeCommand( "L13 exclude11.cdd" );

# Remove temporary exclusion reason file
system( "rm -f exclude11a.excl exclude11b.excl exclude11c.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude11.rptM exclude11.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude11.rptI exclude11.cdd" );

# Perform the file comparison checks
&checkTest( "exclude11", 1, 0 );

exit 0;

