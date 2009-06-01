# Name:     merge4.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/18/2008
# Purpose:  Verifies that if no -o option is specified to the merge command, that the
#           first CDD specified contains the merged results.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge4", 0, @ARGV );

# Run both of the CDDs to be merged
system( "$MAKE DIAG=merge4a onemergerun" ) && die;
system( "$MAKE DIAG=merge4b onemergerun" ) && die;

# Copy the merge4a CDD file to a temporary file (for comparison purposes)
system( "cp merge4a.cdd merge4a.tmp.cdd" ) && die;

# Now perform merge command
&runMergeCommand( "merge4a.cdd merge4b.cdd" );

# Move newly merged merge4a.cdd to merge4.cdd and move merge4a.tmp.cdd to merge4.cdd
system( "mv merge4a.cdd merge4.cdd; mv merge4a.tmp.cdd merge4a.cdd" ) && die;

# Now perform report commands
&runReportCommand( "-d v -o merge4.rptM merge4.cdd" );
&runReportCommand( "-d v -i -o merge4.rptI merge4.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) {
  &checkTest( "merge4", 3, 0 );
} else {
  &checkTest( "merge4", 3, 5 );
}

exit 0;

