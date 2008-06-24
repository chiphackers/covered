# Name:     score_err1.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/23/2008
# Purpose:  Attempts to load the contents of a directory that is lacking read permissions.
#           Verifies that the appropriate error message is displayed to the user.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "score_err1.1", 1, @ARGV );

# Make lib2 directory with read-only permissions
system( "mkdir lib2; chmod 000 lib2" ) && die;

# Perform diagnostic running code here
&runScoreCommand( "-t main -v score_err1.1.v -o score_err1.1.cdd -y lib2 2> score_err1.1.err" );
system( "cat score_err1.1.err" ) && die;

# Remove the directory
system( "chmod 700 lib2; rm -rf lib2" ) && die;

# Perform the file comparison checks
&checkTest( "score_err1.1", 1, 1 );

exit 0;

