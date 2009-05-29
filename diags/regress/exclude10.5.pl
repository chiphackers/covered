# Name:     exclude10.5.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/10/2008
# Purpose:  Runs the exclude command to exclude an assertion for coverage, providing a reason for
#           the exclusion

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude10.5", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -cconfig_file -DDUMP exclude10.5.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP +libext+.v+.vlib+ -y ./ovl +incdir+./ovl +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 exclude10.5.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP +v2k +libext+.v+.vlib+ -y ./ovl +incdir+./ovl +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 exclude10.5.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP +libext+.v+.vlib+ -y ./ovl +incdir+./ovl +define+OVL_VERILOG +define+OVL_COVER_ON +define+OVL_COVER_DEFAULT=15 exclude10.5.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude10.5.vcd -v exclude10.5.v -A ovl -D OVL_COVER_DEFAULT=15 +libext+.vlib+ -y ./ovl -I ./ovl -I ./ovl/vlog95 -o exclude10.5.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This assertion is not needed > exclude10.5.excl" );

# Perform exclusion
&runExcludeCommand( "-m A227 exclude10.5.cdd < exclude10.5.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude10.5.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -m a -e -x -o exclude10.5.rptM exclude10.5.cdd" );
&runReportCommand( "-d v -m a -e -x -i -o exclude10.5.rptI exclude10.5.cdd" );

# Perform the file comparison checks
&checkTest( "exclude10.5", 1, 0 );

exit 0;

