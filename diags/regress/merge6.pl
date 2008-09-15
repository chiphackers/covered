# Name:     merge4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that a two-level merge works properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge6", 0, @ARGV );

# First, create the two subdirectories that will contain the partially merged data contents.
system( "mkdir -p merge6_1 merge6_2" ) && die;

# Run all of the CDDs to be merged
$retval = &run( "merge6a" ) || $retval;
$retval = &run( "merge6b" ) || $retval;
$retval = &run( "merge6c" ) || $retval;
$retval = &run( "merge6d" ) || $retval;

# Save the value of CHECK_MEM_CMD
$ORIG_CHECK_MEM_CMD = $CHECK_MEM_CMD;

# Set the covered and check_mem paths accordingly
$COVERED = "../../../src/covered";
if( $ORIG_CHECK_MEM_CMD ne "" ) {
  $CHECK_MEM_CMD = "| ../check_mem";
}

# Perform merge command of first two CDDs
chdir( "merge6_1" ) || die "Unable to change to merge6_1 directory: $!\n";
&runMergeCommand( "-o merge6.cdd merge6a.cdd merge6b.cdd" );

# Perform merge command of last two CDDs
chdir( "../merge6_2" ) || die "Unable to change to merge6_2 directory: $!\n";
&runMergeCommand( "-o merge6.cdd merge6c.cdd merge6d.cdd" );

# Perform top-most merge
chdir( ".." ) || die "Unable to change to main directory: $!\n";
$COVERED       = "../../src/covered";
$CHECK_MEM_CMD = $ORIG_CHECK_MEM_CMD;
&runMergeCommand( "-o merge6.cdd merge6_1/merge6.cdd merge6_2/merge6.cdd" );

# Remove the temporary directories
system( "rm -rf merge6_1 merge6_2" ) && die;

# Now perform report commands
&runReportCommand( "-d v -o merge6.rptM merge6.cdd" );
&runReportCommand( "-d v -i -o merge6.rptI merge6.cdd" );

# Perform the file comparison checks
&checkTest( "merge6", 5, 0 );

exit 0;

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
    &convertCfg( "vcd", "${bname}.cfg" );
  } elsif( $DUMPTYPE eq "LXT" ) {
    &convertCfg( "lxt", "${bname}.cfg" );
  } else {
    die "Illegal DUMPTYPE value (${DUMPTYPE})\n";
  }

  # Score CDD file
  &runScoreCommand( "-f ${bname}.cfg -D DUMP" );

  return $retval;

}

