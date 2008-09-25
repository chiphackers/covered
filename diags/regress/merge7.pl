# Name:     merge7.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/22/2008
# Purpose:  Merges two CDD files with exclusion reasons (no conflicts).

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge7", 0, @ARGV );

# Simulate the CDD files to merge
&run( "merge7a" ) && die;
&run( "merge7b" ) && die;

# Exclude variable "a" from toggle coverage in merge7a
&runCommand( "echo The output variable a is not being used > merge7a.excl" );
&runExcludeCommand( "-m T01 merge7a.cdd < merge7a.excl" );
system( "rm -f merge7a.excl" ) && die;

# Allow the timestamps to be different
sleep( 1 );

# Exclude variable "B" from toggle coverage in merge7b
&runCommand( "echo The output variable b is not being used > merge7b.excl" );
&runExcludeCommand( "-m T02 merge7b.cdd < merge7b.excl" );
system( "rm -f merge7b.excl" ) && die;

# Perform the merge (don't specify an exclusion reason resolver)
&runMergeCommand( "-o merge7.cdd merge7a.cdd merge7b.cdd" );

# Generate reports
&runReportCommand( "-d v -e -x -o merge7.rptM merge7.cdd" );
&runReportCommand( "-d v -e -x -i -o merge7.rptI merge7.cdd" );

# Perform the file comparison checks
&checkTest( "merge7", 3, 0 );

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

