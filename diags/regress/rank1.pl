# Name:     rank1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     08/04/2008
# Purpose:  Verifies that a non-unique CDD is excluded from ranking.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank1", 0, @ARGV );

# Run all diagnostics
if( $USE_VPI == 0 ) {

  my( $retval ) = 0;

  $retval = &run( "rank1a" ) || $retval;
  $retval = &run( "rank1b" ) || $retval;
  $retval = &run( "rank1c" ) || $retval;

  # Run the rank command (Note that this is NOT an error)
  &runRankCommand( "-o rank1.err rank1a.cdd rank1b.cdd rank1c.cdd" );

  # Check the difference and remove the CDD files, if necessary
  system( "touch rank1.cdd" ) && die;
  &checkTest( "rank1", (($retval == 0) ? 4 : 1), 1 );

}

sub run {

  my( $bname )  = $_[0];
  my( $retval ) = 0;
  my( $fmt )    = "";

  # Convert configuration file
  if( $DUMPTYPE eq "VCD" ) {
    &convertCfg( "vcd", 0, 0, "${bname}.cfg" );
  } elsif( $DUMPTYPE eq "LXT" ) {
    &convertCfg( "lxt", 0, 0, "${bname}.cfg" );
    $fmt = "-lxt2";
  } elsif( $DUMPTYPE eq "FST" ) {
    &convertCfg( "fst", 0, 0, "${bname}.cfg" );
    $fmt = "-fst";
  } else {
    die "Illegal DUMPTYPE value (${DUMPTYPE})\n";
  }

  # Simulate the design
  if( $SIMULATOR eq "IV" ) {
    system( "iverilog -DDUMP -y lib ${bname}.v; ./a.out ${fmt}" ) && die; 
  } elsif( $SIMULATOR eq "CVER" ) {
    system( "cver -q +define+DUMP +libext+.v+ -y lib ${bname}.v" ) && die;
  } elsif( $SIMULATOR eq "VCS" ) {
    system( "vcs +define+DUMP +v2k -sverilog +libext+.v+ -y lib ${bname}.v; ./simv" ) && die; 
  } else {
    die "Illegal SIMULATOR value (${SIMULATOR})\n";
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

