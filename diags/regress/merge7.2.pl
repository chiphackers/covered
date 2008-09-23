# Name:     merge7.2.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/22/2008
# Purpose:  Merges two CDD files with exclusion reasons specified for the same coverage point (conflict).
#           Verifies that the "first" resolution value works properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge7.2", 0, @ARGV );

# Simulate the CDD files to merge
&run( "merge7.2a" ) && die;
&run( "merge7.2b" ) && die;

# Exclude variable "a" from toggle coverage in merge7.1a
&runCommand( "echo The output variable a is not being used > merge7.2a.excl" );
&runExcludeCommand( "-m T01 merge7.2a.cdd < merge7.2a.excl" );
system( "rm -f merge7.2a.excl" ) && die;

# Allow the timestamps to be different
sleep( 1 );

# Exclude variable "a" from toggle coverage (with different message) in merge7.2b
&runCommand( "echo I dont like the variable a > merge7.2b.excl" );
&runExcludeCommand( "-m T01 merge7.2b.cdd < merge7.2b.excl" );
system( "rm -f merge7.2b.excl" ) && die;

# Perform the merge (don't specify an exclusion reason resolver)
&runMergeCommand( "-er first -o merge7.2.cdd merge7.2a.cdd merge7.2b.cdd" );

# Generate reports
&runReportCommand( "-d v -e -x -o merge7.2.rptM merge7.2.cdd" );
&runReportCommand( "-d v -e -x -i -o merge7.2.rptI merge7.2.cdd" );

# Perform the file comparison checks
&checkTest( "merge7.2", 3, 0 );

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

