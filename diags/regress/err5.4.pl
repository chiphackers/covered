# Name:     err5.4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/21/2008
# Purpose:  Modify vector size information so that when this information is read in for merging that
#           we get a CDD mismatch error.
# Note:     If the format of the information line change, this diagnostic will most likely
#           fail and will need to be updated accordingly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err5.4", 1, @ARGV );

# Simulate and get coverage information
system( "$MAKE DIAG=err5.4a oneerrmergerun" ) && die;
system( "$MAKE DIAG=err5.4b oneerrmergerun" ) && die;
system( "mv err5.4b.cdd err5.4b.tmp.cdd" ) && die;

# Modify the version to something which is different
open( OLD_CDD, "err5.4b.tmp.cdd" ) || die "Can't open err5.4b.tmp.cdd for reading: $!\n";
open( NEW_CDD, ">err5.4b.cdd" ) || die "Can't open err5.4b.cdd for writing: $!\n";
while( <OLD_CDD> ) {
  chomp;
  @line = split;
  if( $line[0] eq "5" ) {
    # vsignal outputs 6 items + 2 dimensional items, vector outputs 2 items + data items -- so truncate line to 8 items
    $line[2] = 1;
    print NEW_CDD "@line\n";
  } else {
    print NEW_CDD "$_\n";
  }
}
close( NEW_CDD );
close( OLD_CDD );
system( "rm -f err5.4b.tmp.cdd" ) && die;

# Attempt to run the score command to score it
&runMergeCommand( "err5.4a.cdd err5.4b.cdd 2> err5.4.err" );
system( "cat err5.4.err" ) && die;

# Perform the file comparison checks
&checkTest( "err5.4", 3, 1 );

exit 0;

