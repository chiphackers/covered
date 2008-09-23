# Name:     merge7.8.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/22/2008
# Purpose:  Merges two CDD files with exclusion reasons specified for the same coverage point (conflict).
#           Verifies that the "old" resolution value works properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge7.8", 0, @ARGV );

# Simulate the CDD files to merge
&run( "merge7.8a" ) && die;
&run( "merge7.8b" ) && die;

# Exclude variable "a" from toggle coverage in merge7.8a
&runCommand( "echo The output variable a is not being used > merge7.8a.excl" );
&runExcludeCommand( "-m T01 merge7.8a.cdd < merge7.8a.excl" );
system( "rm -f merge7.8a.excl" ) && die;

# Allow the timestamps to be different
sleep( 1 );

# Exclude variable "a" from toggle coverage (with different message) in merge7.8b
&runCommand( "echo I dont like the variable a > merge7.8b.excl" );
&runExcludeCommand( "-m T01 merge7.8b.cdd < merge7.8b.excl" );
system( "rm -f merge7.8b.excl" ) && die;

# Perform the merge using the last reason found
&runMergeCommand( "-er old -o merge7.8.cdd merge7.8b.cdd merge7.8a.cdd" );

# Generate reports
&runReportCommand( "-d v -e -x -o merge7.8.rptM merge7.8.cdd" );
&runReportCommand( "-d v -e -x -i -o merge7.8.rptI merge7.8.cdd" );

# Perform the file comparison checks
&checkTest( "merge7.8", 3, 0 );

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
  &runScoreCommand( "-f ${bname}.cfg" );

  return $retval;

}

