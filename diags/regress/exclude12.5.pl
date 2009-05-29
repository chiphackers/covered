# Name:     exclude12.5.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/17/2008
# Purpose:  Verify that an excluded assertion coverage point can be output correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude12.5", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP -cconfig_file exclude12.5.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP +libext+.v+.vlib+ -y ./ovl +incdir+./ovl +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 exclude12.5.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP +v2k +libext+.v+.vlib+ -y ./ovl +incdir+./ovl +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 exclude12.5.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP +libext+.v+vlib -y ./ovl +incdir+./ovl +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 exclude12.5.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude12.5.vcd -v exclude12.5.v -A ovl -D OVL_COVER_DEFAULT=15 +libext+.vlib+ -y ./ovl -I ./ovl -I ./ovl/vlog95 -o exclude12.5.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This assertion is not needed > exclude12.5.excl" );

# Perform exclusion
&runExcludeCommand( "-m A227 exclude12.5.cdd < exclude12.5.excl" );

# Now print the exclusion
if( $CHECK_MEM_CMD ne "" ) {
  $check = 1;
  $CHECK_MEM_CMD = "";
}
&runExcludeCommand( "-p A227 exclude12.5.cdd > exclude12.5.out" );
if( $check == 1 ) {
  system( "cat exclude12.5.out | ./check_mem > exclude12.5.err" ) && die;
} else {
  system( "mv exclude12.5.out exclude12.5.err" ) && die;
}
system( "cat exclude12.5.err" ) && die;

# Remove temporary exclusion reason file
system( "rm -f exclude12.5.excl" ) && die;

# Perform the file comparison checks
&checkTest( "exclude12.5", 1, 1 );

exit 0;

