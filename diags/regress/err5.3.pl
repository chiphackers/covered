# Name:     err5.3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/21/2008
# Purpose:  Remove vector information in CDD for a signal and verify
#           that when this information is merged in that we get a parsing error.
# Note:     If the format of the vsignal or vector structures change, this diagnostic will most likely
#           fail and will need to be updated accordingly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err5.3", 1, @ARGV );

# Simulate and get coverage information
system( "$MAKE DIAG=err5.3a oneerrmergerun" ) && die;
system( "$MAKE DIAG=err5.3b oneerrmergerun" ) && die;
system( "mv err5.3b.cdd err5.3b.tmp.cdd" ) && die;

# Modify the version to something which is different
open( OLD_CDD, "err5.3b.tmp.cdd" ) || die "Can't open err5.3b.tmp.cdd for reading: $!\n";
open( NEW_CDD, ">err5.3b.cdd" ) || die "Can't open err5.3b.cdd for writing: $!\n";
while( <OLD_CDD> ) {
  chomp;
  @line = split;
  if( ($line[0] eq "1") && ($line[1] eq "b") ) {
    $#line = 8;
    # vsignal outputs 7 items + 2 dimensional items, vector outputs 2 items + data items -- so truncate line to 9 items
    print NEW_CDD "@line\n";
  } else {
    print NEW_CDD "$_\n";
  }
}
close( NEW_CDD );
close( OLD_CDD );
system( "rm -f err5.3b.tmp.cdd" ) && die;

# Attempt to run the score command to score it
&runMergeCommand( "err5.3a.cdd err5.3b.cdd 2> err5.3.err" );
system( "cat err5.3.err" ) && die;

# Perform the file comparison checks
&checkTest( "err5.3", 3, 1 );

exit 0;

