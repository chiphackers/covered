# Name:     merge7.7.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/22/2008
# Purpose:  Merges two CDD files with exclusion reasons specified for the same coverage point (conflict).
#           Verifies that the "old" resolution value works properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge7.7", 0, @ARGV );

# Simulate the CDD files to merge
&run( "merge7.7a" ) && die;
&run( "merge7.7b" ) && die;

# Exclude variable "a" from toggle coverage in merge7.7a
&runCommand( "echo The output variable a is not being used > merge7.7a.excl" );
&runExcludeCommand( "-m T01 merge7.7a.cdd < merge7.7a.excl" );
system( "rm -f merge7.7a.excl" ) && die;

# Allow the timestamps to be different
sleep( 1 );

# Exclude variable "a" from toggle coverage (with different message) in merge7.7b
&runCommand( "echo I dont like the variable a > merge7.7b.excl" );
&runExcludeCommand( "-m T01 merge7.7b.cdd < merge7.7b.excl" );
system( "rm -f merge7.7b.excl" ) && die;

# Perform the merge using the last reason found
&runMergeCommand( "-er old -o merge7.7.cdd merge7.7a.cdd merge7.7b.cdd" );

# Generate reports
&runReportCommand( "-d v -e -x -o merge7.7.rptM merge7.7.cdd" );
&runReportCommand( "-d v -e -x -i -o merge7.7.rptI merge7.7.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) {
  &checkTest( "merge7.7", 3, 0 ); 
} else {
  &checkTest( "merge7.7", 3, 5 );
} 

exit 0;


sub run {

  my( $bname )  = $_[0];
  my( $retval ) = 0;
  my( $fmt )    = "";

  # If we are using the VPI, run the score command and add the needed pieces to the simulation runs
  if( $USE_VPI == 1 ) {
    &convertCfg( "vpi", 0, 0, "${bname}.cfg" );
    &runScoreCommand( "-f ${bname}.cfg" );
    $vpi_args = "+covered_cdd=${bname}.cdd";
    if( $COVERED_GFLAGS eq "-D" ) {
      $vpi_args .= " +covered_debug";
    } 
  } 
  
  # Simulate the design
  if( $SIMULATOR eq "IV" ) {
    if( $USE_VPI == 1 ) {
      &runCommand( "iverilog -y lib -m ../../lib/vpi/covered.vpi ${bname}.v covered_vpi.v; ./a.out ${vpi_args}" );
    } else {
      &runCommand( "iverilog -DDUMP -y lib ${bname}.v; ./a.out" );
    }
  } elsif( $SIMULATOR eq "CVER" ) {
    if( $USE_VPI == 1 ) {
      &runCommand( "cver -q +libext+.v+ -y lib +loadvpi=../../lib/vpi/covered.cver.so:vpi_compat_bootstrap ${bname}.v covered_vpi.v ${vpi_args}" );
    } else {
      &runCommand( "cver -q +define+DUMP +libext+.v+ -y lib ${bname}.v" );
    }
  } elsif( $SIMULATOR eq "VCS" ) {
    if( $USE_VPI == 1 ) {
      &runCommand( "vcs +v2k -sverilog +libext+.v+ -y lib +vpi -load ../../lib/vpi/covered.vcs.so:covered_register ${bname}.v covered_vpi.v; ./simv ${vpi_args}" );
    } else {
      &runCommand( "vcs +define+DUMP +v2k -sverilog +libext+.v+ -y lib ${bname}.v; ./simv" );
    }
  } else {
    die "Illegal SIMULATOR value (${SIMULATOR})\n";
  }

  # If we are doing VCD/LXT simulation, run the score command post-process
  if( $USE_VPI == 0 ) {

    # Convert configuration file
    if( $DUMPTYPE eq "VCD" ) {
      &convertCfg( "vcd", 0, 0, "${bname}.cfg" );
    } elsif( $DUMPTYPE eq "LXT" ) {
      &convertCfg( "lxt", 0, 0, "${bname}.cfg" );
    }

    # Score CDD file
    &runScoreCommand( "-f ${bname}.cfg -D DUMP" );

  }

  return $retval;

}

