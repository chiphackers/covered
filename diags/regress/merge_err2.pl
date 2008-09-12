# Name:     merge_err2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     08/04/2008
# Purpose:  Verifies that an error is displayed if a file is attempted to be merged more than once.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge_err2", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP merge_err2.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP merge_err2.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP merge_err2.v; ./simv" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -v merge_err2.v -o merge_err2.cdd" );
&runScoreCommand( "-t main -vcd merge_err2.vcd -v merge_err2.v -o merge_err2.full.cdd" );

&runMergeCommand( "merge_err2.cdd merge_err2.full.cdd" );
&runMergeCommand( "merge_err2.cdd merge_err2.full.cdd 2> merge_err2.err" );
system( "cat merge_err2.err" ) && die;
system( "rm -rf merge_err2.full.cdd" ) && die;

# Perform the file comparison checks
&checkTest( "merge_err2", 1, 1 );

exit 0;

