# Name:     err6.4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     07/28/2008
# Purpose:  Remove portion of fsm_arc structure from FSM output that will cause a parsing error to occur.
# Note:     If the format of the fsm or fsm_arc structures change, this diagnostic will most likely
#           fail and will need to be updated accordingly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "err6.4", 1, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) {
  system( "iverilog -DDUMP -y lib err6.4.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP -y lib err6.4.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP -y lib err6.4.v; ./simv" ) && die;
}

# Create initial "good" CDD file
&runScoreCommand( "-t main -v err6.4.v -y lib -o err6.4.tmp.cdd" );

# Modify the version to something which is different
open( OLD_CDD, "err6.4.tmp.cdd" ) || die "Can't open err6.4.tmp.cdd for reading: $!\n";
open( NEW_CDD, ">err6.4.cdd" ) || die "Can't open err6.4.cdd for writing: $!\n";
while( <OLD_CDD> ) {
  chomp;
  @line = split;
  if( $line[0] eq "6" ) {
    # fsm outputs 4 + fsm_arc outputs 3 items + 4 vector items (per to_state and from_state) + 1 num_arcs item + 3 items per state transition
    $#line = 5;
    print "line: @line\n";
    print NEW_CDD "@line\n";
  } else {
    print NEW_CDD "$_\n";
  }
}
close( NEW_CDD );
close( OLD_CDD );
system( "rm -f err6.4.tmp.cdd" ) && die;

# Attempt to run the score command to score it
&runScoreCommand( "-t main -vcd err6.4.vcd -cdd err6.4.cdd 2> err6.4.err" );
system( "cat err6.4.err" ) && die;

# Perform the file comparison checks
&checkTest( "err6.4", 1, 1 );

exit 0;

