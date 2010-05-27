# Name:     merge5.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     07/01/2008
# Purpose:  Performs 7 CDD ranking

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge5", 0, @ARGV );

# Run all diagnostics
if( $USE_VPI == 0 ) {

  my( $retval ) = 0;

  $retval = &run( "merge5a" ) || $retval;
  $retval = &run( "merge5b" ) || $retval;
  $retval = &run( "merge5c" ) || $retval;
  $retval = &run( "merge5d" ) || $retval;
  $retval = &run( "merge5e" ) || $retval;
  $retval = &run( "merge5f" ) || $retval;
  $retval = &run( "merge5g" ) || $retval;

  # Run the rank command (Note that this is NOT an error)
  &runRankCommand( "-o merge5.err merge5a.cdd merge5b.cdd merge5c.cdd merge5d.cdd merge5e.cdd merge5f.cdd merge5g.cdd" );

  # Check the difference and remove the CDD files, if necessary
  system( "touch merge5.cdd" ) && die;
  &checkTest( "merge5", (($retval == 0) ? 8 : 1), 1 );

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

