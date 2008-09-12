# Name:     err5.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/21/2008
# Purpose:  Remove data/coverage portion of vector information in CDD for a signal and verify
#           that when this information is read in that we get a parsing error.
# Note:     If the format of the vsignal or vector structures change, this diagnostic will most likely
#           fail and will need to be updated accordingly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err5", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP err5.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP err5.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP err5.v; ./simv" ) && die;
}

# Create initial "good" CDD file
&runScoreCommand( "-t main -v err5.v -o err5.tmp.cdd" );

# Modify the version to something which is different
open( OLD_CDD, "err5.tmp.cdd" ) || die "Can't open err5.tmp.cdd for reading: $!\n";
open( NEW_CDD, ">err5.cdd" ) || die "Can't open err5.cdd for writing: $!\n";
while( <OLD_CDD> ) {
  chomp;
  @line = split;
  if( $line[0] eq "1" ) {
    $#line = 9;
    # vsignal outputs 6 items + 2 dimensional items, vector outputs 2 items + data items -- so truncate line to 8 items
    print NEW_CDD "@line\n";
  } else {
    print NEW_CDD "$_\n";
  }
}
close( NEW_CDD );
close( OLD_CDD );
system( "rm -f err5.tmp.cdd" ) && die;

# Attempt to run the score command to score it
&runScoreCommand( "-t main -vcd err5.vcd -cdd err5.cdd 2> err5.err" );
system( "cat err5.err" ) && die;

# Perform the file comparison checks
&checkTest( "err5", 1, 1 );

exit 0;

