# Name:     inlined_err1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     07/14/2009
# Purpose:  Attempts to use a CDD file which is not setup for inlined coverage from a VCD file that
#           contains inlined coverage information. 

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "inlined_err1", 1, @ARGV );

if( ($SIMULATOR eq "IV") && (($DUMPTYPE eq "LXT") || ($DUMPTYPE eq "VCD")) ) {

  # Perform initial score for inlined code coverage
  &runScoreCommand( "-t main -inline -v inlined_err1.v -o inlined_err1.cdd -D DUMP" );

  if( $DUMPTYPE eq "VCD" ) {
    $dump_opt = "-vcd";
    $iv_opt   = "";
  } else {
    $dump_opt = "-lxt";
    $iv_opt   = "-lxt2";
  }

  # Compile and run this diagnostic using IV
  &runCommand( "iverilog -DDUMP covered/verilog/inlined_err1.v; ./a.out $iv_opt" );

  # Perform second score for no inlined code coverage using VCD from inlined run
  &runScoreCommand( "-t main $dump_opt inlined_err1.vcd -v inlined_err1.v -o inlined_err1.cdd 2> inlined_err1.err" );

  system( "cat inlined_err1.err" ) && die;

  # Perform the file comparison checks
  &checkTest( "inlined_err1", 1, 1 );

}

exit 0;

