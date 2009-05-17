# Name:     exclude12.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  Verify that an excluded toggle coverage point can be output correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude12.1", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude12.1.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude12.1.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude12.1.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude12.1.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude12.1.vcd -v exclude12.1.v -o exclude12.1.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This toggle is not needed > exclude12.1.excl" );

# Perform exclusion
&runExcludeCommand( "-m T01 exclude12.1.cdd < exclude12.1.excl" );

# Now print the exclusion
if( $CHECK_MEM_CMD ne "" ) {
  $check = 1;
  $CHECK_MEM_CMD = "";
}
&runExcludeCommand( "-p T01 exclude12.1.cdd > exclude12.1.out" );
if( $check == 1 ) {
  system( "cat exclude12.1.out | ./check_mem > exclude12.1.err" ) && die;
} else {
  system( "mv exclude12.1.out exclude12.1.err" ) && die;
}
system( "cat exclude12.1.err" ) && die;

# Remove temporary exclusion reason file
system( "rm -f exclude12.1.excl" ) && die;

# Perform the file comparison checks
&checkTest( "exclude12.1", 1, 1 );

exit 0;

