# Name:     exclude_err2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that a call to the exclude command with an invalid first letter
#           outputs the appropriate error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude_err2", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude_err2.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude_err2.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude_err2.v; ./simv" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude_err2.vcd -v exclude_err2.v -o exclude_err2.cdd" );

# Perform diagnostic running code here
&runExcludeCommand( "X01 exclude_err2.cdd 2> exclude_err2.err" );
system( "cat exclude_err2.err" ) && die;

system( "rm -f exclude_err2.cdd" ) && die;

# Perform the file comparison checks
&checkTest( "exclude_err2", 1, 1 );

exit 0;

