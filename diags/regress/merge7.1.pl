# Name:     merge7.1.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/22/2008
# Purpose:  Merges two CDD files with exclusion reasons specified for the same coverage point (but are the same
#           reason.  Verifies that no conflicts are detected.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge7.1", 0, @ARGV );

# Simulate the CDD files to merge
&run( "merge7.1a" ) && die;
&run( "merge7.1b" ) && die;

# Exclude variable "a" from toggle coverage in merge7a
&runCommand( "echo The output variable a is not being used > merge7.1.excl" );
&runExcludeCommand( "-m T01 merge7.1a.cdd < merge7.1.excl" );

# Allow the timestamps to be different
sleep( 1 );

# Exclude variable "B" from toggle coverage in merge7b
&runExcludeCommand( "-m T01 merge7.1b.cdd < merge7.1.excl" );
system( "rm -f merge7.1.excl" ) && die;

# Perform the merge (don't specify an exclusion reason resolver)
&runMergeCommand( "-o merge7.1.cdd merge7.1a.cdd merge7.1b.cdd" );

# Generate reports
&runReportCommand( "-d v -e -x -o merge7.1.rptM merge7.1.cdd" );
&runReportCommand( "-d v -e -x -i -o merge7.1.rptI merge7.1.cdd" );

# Perform the file comparison checks
&checkTest( "merge7.1", 3, 0 );

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

