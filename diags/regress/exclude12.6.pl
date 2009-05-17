# Name:     exclude12.6.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  Verify that an included line coverage point can be printed properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude12.6", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude12.6.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude12.6.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude12.6.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude12.6.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude12.6.vcd -v exclude12.6.v -o exclude12.6.cdd" );

# Create temporary file that will contain an exclusion message
system( "touch exclude12.6.excl" ) && die;
  
# Perform exclusion
&runExcludeCommand( "-m L10 exclude12.6.cdd < exclude12.6.excl" );

# Now print the exclusion
if( $CHECK_MEM_CMD ne "" ) {
  $check = 1;
  $CHECK_MEM_CMD = "";
}
&runExcludeCommand( "-p L10 exclude12.6.cdd > exclude12.6.out" );
if( $check == 1 ) {
  system( "cat exclude12.6.out | ./check_mem > exclude12.6.err" ) && die;
} else {
  system( "mv exclude12.6.out exclude12.6.err" ) && die;
}
system( "cat exclude12.6.err" ) && die;

# Remove temporary exclusion reason file
system( "rm -f exclude12.6.excl" ) && die;

# Perform the file comparison checks
&checkTest( "exclude12.6", 1, 1 );

exit 0;

