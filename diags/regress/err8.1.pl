# Name:     err8.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/21/2008
# Purpose:  Verify that if the information line is formatted incorrectly,
#           an appropriate error message is output.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err8.1", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP err8.1.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP err8.1.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP err8.1.v; ./simv" ) && die;
}

# Create CDD file that we will modify the version of
&runScoreCommand( "-t main -vcd err8.1.vcd -v err8.v -o err8.1.tmp.cdd" );

# Modify the version to something which is different
open( OLD_CDD, "err8.1.tmp.cdd" ) || die "Can't open err8.1.tmp.cdd for reading: $!\n";
open( NEW_CDD, ">err8.1.cdd" ) || die "Can't open err8.1.cdd for writing: $!\n";
while( <OLD_CDD> ) {
  chomp;
  if( /^5\s+(.*)$/ ) {
    print NEW_CDD "5\n";
  } else {
    print NEW_CDD "$_\n";
  }
}
close( NEW_CDD );
close( OLD_CDD );
system( "rm -f err8.1.tmp.cdd" ) && die;

&runReportCommand( "-d v -m ltcfam err8.1.cdd 2> err8.1.err" );
system( "cat err8.1.err" ) && die;

# Perform the file comparison checks
&checkTest( "err8.1", 1, 1 );

exit 0;

