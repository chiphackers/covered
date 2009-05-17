# Name:     exclude12.4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  Verify that an excluded FSM coverage point can be output correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude12.4", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP -y lib exclude12.4.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP +libext+.v+.vlib+ -y lib exclude12.4.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP +libext+.v+.vlib+ -y lib exclude12.4.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP +libext+.v+.vlib -y lib exclude12.4.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude12.4.vcd -v exclude12.4.v -y lib -o exclude12.4.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This FSM is not needed > exclude12.4.excl" );

# Perform exclusion
&runExcludeCommand( "-m F002 exclude12.4.cdd < exclude12.4.excl" );

# Now print the exclusion
if( $CHECK_MEM_CMD ne "" ) {
  $check = 1;
  $CHECK_MEM_CMD = "";
}
&runExcludeCommand( "-p F002 exclude12.4.cdd > exclude12.4.out" );
if( $check == 1 ) {
  system( "cat exclude12.4.out | ./check_mem > exclude12.4.err" ) && die;
} else {
  system( "mv exclude12.4.out exclude12.4.err" ) && die;
}
system( "cat exclude12.4.err" ) && die;

# Remove temporary exclusion reason file
system( "rm -f exclude12.4.excl" ) && die;

# Perform the file comparison checks
&checkTest( "exclude12.4", 1, 1 );

exit 0;

