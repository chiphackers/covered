# Name:     rank_err4.3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that a -required-list file that contains a non-existent filename
#           within it outputs the needed error message.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank_err4.3", 1, @ARGV );

# Create -required-list file
open( FILE, ">rank_err4.3.in" ) || die "Can't open rank_err4.3.in for writing: $!\n";
print FILE dequote(<<"EOPRT");
@@@ rank_err4.3a.cdd
@@@ rank_err4.3b.cdd
EOPRT
close FILE;

# Create the first CDD file
system( "touch rank_err4.3a.cdd" ) && die;

# Perform diagnostic running code here
&runRankCommand( "-required-list rank_err4.3.in 2> rank_err4.3.err" );
system( "cat rank_err4.3.err" ) && die;

# Remove temporary files
system( "rm -f rank_err4.3a.cdd" ) && die;

# Perform the file comparison checks
&checkTest( "rank_err4.3", 1, 1 );

exit 0;

