# Name:     merge7.3.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/22/2008
# Purpose:  Merges two CDD files with exclusion reasons specified for the same coverage point (conflict).
#           Verifies that the "last" resolution value works properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge7.3", 0, @ARGV );

# Simulate the CDD files to merge
&run( "merge7.3a" ) && die;
&run( "merge7.3b" ) && die;

# Exclude variable "a" from toggle coverage in merge7.3a
&runCommand( "echo The output variable a is not being used > merge7.3a.excl" );
&runExcludeCommand( "-m T01 merge7.3a.cdd < merge7.3a.excl" );
system( "rm -f merge7.3a.excl" ) && die;

# Allow the timestamps to be different
sleep( 1 );

# Exclude variable "a" from toggle coverage (with different message) in merge7.3b
&runCommand( "echo I dont like the variable a > merge7.3b.excl" );
&runExcludeCommand( "-m T01 merge7.3b.cdd < merge7.3b.excl" );
system( "rm -f merge7.3b.excl" ) && die;

# Perform the merge using the last reason found
&runMergeCommand( "-er last -o merge7.3.cdd merge7.3a.cdd merge7.3b.cdd" );

# Generate reports
&runReportCommand( "-d v -e -x -o merge7.3.rptM merge7.3.cdd" );
&runReportCommand( "-d v -e -x -i -o merge7.3.rptI merge7.3.cdd" );

# Perform the file comparison checks
&checkTest( "merge7.3", 3, 0 );

exit 0;


sub run {

  my( $bname )  = $_[0];
  my( $retval ) = 0;
  my( $fmt )    = "";

  # Convert configuration file
  if( $DUMPTYPE eq "VCD" ) {
    &convertCfg( "vcd", "${bname}.cfg" );
  } elsif( $DUMPTYPE eq "LXT" ) {
    &convertCfg( "lxt", "${bname}.cfg" );
    $fmt = "-lxt2";
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

  return $retval;

}

