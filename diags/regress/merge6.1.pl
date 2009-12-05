# Name:     merge6.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     08/16/2008
# Purpose:  Verifies that a two-level merge works properly even when a file
#           is named the same in two different directories.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge6.1", 0, @ARGV );

# First, create the two subdirectories that will contain the partially merged data contents.
system( "mkdir -p merge6_1 merge6_2" ) && die;

# Run all of the CDDs to be merged
$retval = &run( "merge6.1a", "merge6.1a", "merge6_1", 1 ) || $retval;
$retval = &run( "merge6.1b", "merge6.1b", "merge6_1", 0 ) || $retval;
$retval = &run( "merge6.1c", "merge6.1a", "merge6_2", 1 ) || $retval;
$retval = &run( "merge6.1d", "merge6.1b", "merge6_2", 0 ) || $retval;

# Save the value of CHECK_MEM_CMD
$ORIG_CHECK_MEM_CMD = $CHECK_MEM_CMD;

# Set the covered and check_mem paths accordingly
$COVERED = "../../../src/covered";
if( $ORIG_CHECK_MEM_CMD ne "" ) {
  $CHECK_MEM_CMD = "| ../check_mem";
}

# Perform merge command of first two CDDs
chdir( "merge6_1" ) || die "Unable to change to merge6_1 directory: $!\n";
&runMergeCommand( "-o merge6.1.cdd merge6.1a.cdd merge6.1b.cdd" );

# Perform merge command of last two CDDs
chdir( "../merge6_2" ) || die "Unable to change to merge6_2 directory: $!\n";
&runMergeCommand( "-o merge6.1.cdd merge6.1a.cdd merge6.1b.cdd" );

# Perform top-most merge
chdir( ".." ) || die "Unable to change to main directory: $!\n";
$COVERED       = "../../src/covered";
$CHECK_MEM_CMD = $ORIG_CHECK_MEM_CMD;
&runMergeCommand( "-o merge6.1.cdd merge6_1/merge6.1.cdd merge6_2/merge6.1.cdd" );

# Remove the temporary directories
system( "rm -rf merge6_1 merge6_2" ) && die;

# Now perform report commands
&runReportCommand( "-d v -o merge6.1.rptM merge6.1.cdd" );
&runReportCommand( "-d v -i -o merge6.1.rptI merge6.1.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) {
  &checkTest( "merge6.1", 5, 0 );
} else {
  &checkTest( "merge6.1", 5, 5 );
}   

exit 0;

sub run {

  my( $cfg, $vname, $odir, $d ) = @_;
  my( $retval )       = 0;
  my( $vpi_debug )    = "";

  # If we are using the VPI, run the score command and add the needed pieces to the simulation runs
  if( $USE_VPI == 1 ) {
    &convertCfg( "vpi", 0, 0, "${cfg}.cfg" );
    &runScoreCommand( "-f ${cfg}.cfg" );
    $vpi_args = "+covered_cdd=${odir}/${vname}.cdd";
    if( $COVERED_GFLAGS eq "-D" ) {
      $vpi_args .= " +covered_debug";
    }
  }
  
  # Simulate the design
  if( $SIMULATOR eq "IV" ) {
    $def = ($d == 1) ? "-DTEST1" : "";
    if( $USE_VPI == 1 ) {
      &runCommand( "iverilog $def -y lib -m ../../lib/vpi/covered.vpi ${vname}.v covered_vpi.v; ./a.out ${vpi_args}" );
    } else {
      &runCommand( "iverilog $def -DDUMP -y lib ${vname}.v; ./a.out" );
    }
  } elsif( $SIMULATOR eq "CVER" ) {
    $def = ($d == 1) ? "+define+TEST1" : "";
    if( $USE_VPI == 1 ) {
      &runCommand( "cver -q $def +libext+.v+ -y lib +loadvpi=../../lib/vpi/covered.cver.so:vpi_compat_bootstrap ${vname}.v covered_vpi.v ${vpi_args}" );
    } else {
      &runCommand( "cver -q $def +define+DUMP +libext+.v+ -y lib ${vname}.v" );
    }
  } elsif( $SIMULATOR eq "VCS" ) {
    $def = ($d == 1) ? "+define+TEST1" : "";
    if( $USE_VPI == 1 ) {
      &runCommand( "vcs $def +v2k -sverilog +libext+.v+ -y lib +vpi -load ../../lib/vpi/covered.vcs.so:covered_register ${vname}.v covered_vpi.v; ./simv ${vpi_args}" );
    } else {
      &runCommand( "vcs $def +define+DUMP +v2k -sverilog +libext+.v+ -y lib ${vname}.v; ./simv" );
    }
  } else {
    die "Illegal SIMULATOR value (${SIMULATOR})\n";
  }
  
  # If we are doing VCD/LXT simulation, run the score command post-process
  if( $USE_VPI == 0 ) {
    
    # Convert configuration file
    if( $DUMPTYPE eq "VCD" ) {
      &convertCfg( "vcd", 0, 0, "${cfg}.cfg" );
    } elsif( $DUMPTYPE eq "LXT" ) {
      &convertCfg( "lxt", 0, 0, "${cfg}.cfg" );
    }
    
    # Score CDD file
    &runScoreCommand( "-f ${cfg}.cfg -D DUMP" );
  
  }

  return $retval;

}

