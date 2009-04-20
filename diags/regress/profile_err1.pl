# Name:     profile_err1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     11/24/2008
# Purpose:  Verify that the appropriate error is output if a profiling output file is unwritable.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "profile_err1", 1, @ARGV );

# Create a file called covered.prof and make it read-only.
system( "touch covered.prof; chmod 400 covered.prof" ) && die;

# Perform diagnostic running code here (the command-line doesn't really matter.
$COVERED_SCORE_GFLAGS .= " -P";
&runScoreCommand( "-h 2> profile_err1.err" );
system( "cat profile_err1.err" ) && die;

# Remove the covered.prof file
system( "chmod 600 covered.prof; rm -f covered.prof" );

# Perform the file comparison checks
&checkTest( "profile_err1", 0, 1 );

exit 0;

