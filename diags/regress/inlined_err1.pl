# Name:     inlined_err1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     07/14/2009
# Purpose:  Attempts to use a CDD file which is not setup for inlined coverage from a VCD file that
#           contains inlined coverage information. 

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "inlined_err1", 1, @ARGV );

# Perform initial score for inlined code coverage
&runScoreCommand( "-t main -inline -v inlined_err1.v -o inlined_err1.cdd -D DUMP" );

# Compile and run this diagnostic using IV
&runCommand( "iverilog -DDUMP covered/verilog/inlined_err1.v; ./a.out" );

# Perform second score for no inlined code coverage using VCD from inlined run
&runScoreCommand( "-t main -vcd inlined_err1.vcd -v inlined_err1.v -o inlined_err1.cdd 2> inlined_err1.err" );

system( "cat inlined_err1.err" ) && die;

# Perform the file comparison checks
&checkTest( "inlined_err1", 1, 1 );

exit 0;

