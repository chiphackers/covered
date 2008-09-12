# Name:     err8.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/21/2008
# Purpose:  Verify that when the CDD version does not match the current version of Covered
#           that the appropriate error message is displayed to the user.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err8", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP err8.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP err8.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP err8.v; ./simv" ) && die;
}

# Create CDD file that we will modify the version of
&runScoreCommand( "-t main -vcd err8.vcd -v err8.v -o err8.tmp.cdd" );

# Modify the version to something which is different
open( OLD_CDD, "err8.tmp.cdd" ) || die "Can't open err8.tmp.cdd for reading: $!\n";
open( NEW_CDD, ">err8.cdd" ) || die "Can't open err8.cdd for writing: $!\n";
while( <OLD_CDD> ) {
  chomp;
  if( /^5\s+(\w+)\s+(.*)$/ ) {
    $version = hex( $1 ) - 1;
    $rest    = $2;
    print NEW_CDD "5 " . sprintf( "%x", $version ) . " ${rest}\n";
  } else {
    print NEW_CDD "$_\n";
  }
}
close( NEW_CDD );
close( OLD_CDD );
system( "rm -f err8.tmp.cdd" ) && die;

&runReportCommand( "-d v -m ltcfam err8.cdd 2> err8.err" );
system( "cat err8.err" ) && die;

# Perform the file comparison checks
&checkTest( "err8", 1, 1 );

exit 0;

