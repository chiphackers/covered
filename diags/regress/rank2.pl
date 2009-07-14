# Name:     rank2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verifies that the -v option does what it is supposed to do.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank2", 0, @ARGV );

# Run all diagnostics
if( $USE_VPI == 0 ) {

  my( $retval ) = 0;

  $retval = &run( "rank2a" ) || $retval;
  $retval = &run( "rank2b" ) || $retval;
  $retval = &run( "rank2c" ) || $retval;

  # Turn off debug output
  $COVERED_GFLAGS = "";

  if( $CHECK_MEM_CMD ne "" ) {
    $check = 1;
    $CHECK_MEM_CMD = "";
  }

  # Run the rank command (Note that this is NOT an error)
  &runRankCommand( "-v -o rank2.rpt rank2a.cdd rank2b.cdd rank2c.cdd > rank2.out" );
  system( "rm -f rank2.rpt" ) && die;

  if( $check == 1 ) {
    system( "cat rank2.out | ./check_mem > rank2.err" ) && die;
    system( "rm -f rank2.out" ) && die;
  }

  # Check the difference and remove the CDD files, if necessary
  system( "touch rank2.cdd" ) && die;
  &checkTest( "rank2", (($retval == 0) ? 4 : 1), 1 );

}

sub run {

  my( $bname )  = $_[0];
  my( $retval ) = 0;

  # Simulate the design
  if( $SIMULATOR eq "IV" ) {
    system( "iverilog -DDUMP -y lib ${bname}.v; ./a.out" ) && die; 
  } elsif( $SIMULATOR eq "CVER" ) {
    system( "cver -q +define+DUMP +libext+.v+ -y lib ${bname}.v" ) && die;
  } elsif( $SIMULATOR eq "VCS" ) {
    system( "vcs +define+DUMP +v2k -sverilog +libext+.v+ -y lib ${bname}.v; ./simv" ) && die; 
  } else {
    die "Illegal SIMULATOR value (${SIMULATOR})\n";
  }

  # Convert configuration file
  if( $DUMPTYPE eq "VCD" ) {
    &convertCfg( "vcd", 0, 0, "${bname}.cfg" );
  } elsif( $DUMPTYPE eq "LXT" ) {
    &convertCfg( "lxt", 0, 0, "${bname}.cfg" );
  } else {
    die "Illegal DUMPTYPE value (${DUMPTYPE})\n";
  }

  # Score CDD file
  &runScoreCommand( "-f ${bname}.cfg" );

  # Check that the CDD file matches
  if( $DUMPTYPE eq "VCD" ) {
    $retval = &checkTest( $bname, 0, 0 );
  } else {
    $retval = &checkTest( $bname, 0, 5 );
  }

  return $retval;

}

exit 0;

