# Name:     rank1.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     08/04/2008
# Purpose:  Verifies that a required CDD file is ranked, even if it contains no unique coverage points

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "rank1.1", 0, @ARGV );

# Run all diagnostics
if( $USE_VPI == 0 ) {

  my( $retval ) = 0;

  $retval = &run( "rank1.1a" ) || $retval;
  $retval = &run( "rank1.1b" ) || $retval;
  $retval = &run( "rank1.1c" ) || $retval;

  # Run the rank command (Note that this is NOT an error)
  &runRankCommand( "-required-list ../regress/rank1.1.required -o rank1.1.err rank1.1a.cdd rank1.1c.cdd" );

  # Check the difference and remove the CDD files, if necessary
  system( "touch rank1.1.cdd" ) && die;
  &checkTest( "rank1.1", (($retval == 0) ? 4 : 1), 1 );

}

sub run {

  my( $bname )  = $_[0];
  my( $retval ) = 0;

  # Simulate the design
  if( $SIMULATOR eq "IV" ) {
    system( "iverilog -DDUMP -y lib ${bname}.v; ./a.out" ) && die; 
  } elsif( $SIMULATOR eq "CVER" ) {
    system( "cver +define+DUMP +libext+.v+ -y lib ${bname}.v" ) && die;
  } elsif( $SIMULATOR eq "VCS" ) {
    system( "vcs +define+DUMP +v2k -sverilog +libext+.v+ -y lib ${bname}.v; ./simv" ) && die; 
  } else {
    die "Illegal SIMULATOR value (${SIMULATOR})\n";
  }

  # Convert configuration file
  if( $DUMPTYPE eq "VCD" ) {
    &convertCfg( "vcd", "${bname}.cfg" );
  } elsif( $DUMPTYPE eq "LXT" ) {
    &convertCfg( "lxt", "${bname}.cfg" );
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

