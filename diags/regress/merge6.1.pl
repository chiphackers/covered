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
$retval = &run( "merge6.1a", 1 ) || $retval;
$retval = &run( "merge6.1b", 0 ) || $retval;
$retval = &run( "merge6.1c", 1 ) || $retval;
$retval = &run( "merge6.1d", 0 ) || $retval;

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
&checkTest( "merge6.1", 5, 0 );

exit 0;

sub run {

  my( $bname, $d ) = @_;
  my( $retval ) = 0;

  # Simulate the design
  if( $SIMULATOR eq "IV" ) {
    $def = ($d == 1) ? "-DTEST1" : "";
    system( "iverilog -DDUMP $def -y lib ${bname}.v; ./a.out" ) && die; 
  } elsif( $SIMULATOR eq "CVER" ) {
    $def = ($d == 1) ? "+define+TEST1" : "";
    system( "cver -q +define+DUMP $def +libext+.v+ -y lib ${bname}.v" ) && die;
  } elsif( $SIMULATOR eq "VCS" ) {
    $def = ($d == 1) ? "+define+TEST1" : "";
    system( "vcs +define+DUMP $def +v2k -sverilog +libext+.v+ -y lib ${bname}.v; ./simv" ) && die; 
  } else {
    die "Illegal SIMULATOR value (${SIMULATOR})\n";
  }

  # Convert configuration file
  if( $DUMPTYPE eq "VCD" ) {
    &convertCfg( "vcd", "${bname}.cfg" );
  } elsif( $DUMPTYPE eq "LXT" ) {
    &convertCfg( "lxt", "${bname}.cfg" );
  }

  # Score CDD file
  &runScoreCommand( "-f ${bname}.cfg -D DUMP" );

  return $retval;

}

