# Name:     exclude13.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify that a long exclusion message is handled correctly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude13", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP exclude13.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP exclude13.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP exclude13.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP exclude13.v" ) && die;
}

# Perform diagnostic running code here
&runScoreCommand( "-t main -vcd exclude13.vcd -v exclude13.v -o exclude13.cdd" );

# Create temporary file that will contain an exclusion message
open( MSG, ">exclude13.excl" ) || die "Can't open file exclude13.excl for writing: $!\n";
print MSG dequote(<<"EOPRT");
@@@ This is a very long exclusion message.  I don't know why I need
@@@ to make this reason so long but it's the only way to verify	that Covered
@@@ can handle the long exclusion message.  Hopefully, there's no problem with it!
EOPRT
close MSG;

# Perform exclusion
&runExcludeCommand( "-m L10 exclude13.cdd < exclude13.excl" );

# Now print the exclusion
if( $CHECK_MEM_CMD ne "" ) {
  $check = 1;
  $CHECK_MEM_CMD = "";
}
&runExcludeCommand( "-p L10 exclude13.cdd > exclude13.out" );
if( $check == 1 ) {
  system( "cat exclude13.out | ./check_mem > exclude13.err" ) && die;
} else {
  system( "mv exclude13.out exclude13.err" ) && die;
}
system( "cat exclude13.err" ) && die;

# Remove temporary exclusion reason file
system( "rm -f exclude13.excl" ) && die;

# Perform the file comparison checks
&checkTest( "exclude13", 1, 1 );

exit 0;

