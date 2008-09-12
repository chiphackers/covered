# Name:     err9.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/22/2008
# Purpose:  Hit a compatibility problem with a CDD file and verify that
#           an appropriate error message to the user.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err9", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP err9.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP err9.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP err9.v; ./simv" ) && die;
}

# Create scored CDD file
&runScoreCommand( "-t main -vcd err9.vcd -v err9.v -o err9.tmp.cdd" );

# Modify the CDD file to cause a merged syntax error
open( OLD_CDD, "err9.tmp.cdd" ) || die "Can't open err9.tmp.cdd for reading: $!\n";
open( NEW_CDD, ">err9.cdd" ) || die "Can't open err9.cdd for writing: $!\n";
while( <OLD_CDD> ) {
  chomp;
  if( /^5\s+(\w+)\s+(\w+)\s+(\d+)\s+.*$/ ) {
    $version   = $1;
    $suppl     = $2;
    $timesteps = $3;
    print NEW_CDD "5 $version $suppl $timesteps\n";
  } else {
    print NEW_CDD $_ . "\n";
  }
}
close( NEW_CDD );
close( OLD_CDD );
system( "rm -f err9.tmp.cdd" ) && die;
&runReportCommand( "-d v -o err9.rptM err9.cdd 2> err9.err" );
system( "cat err9.err" ) && die;
system( "rm -f err9.rptM" ) && die;

# Perform the file comparison checks
&checkTest( "err9", 1, 1 );

exit 0;

