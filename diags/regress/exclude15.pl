# Name:     exclude15.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     09/18/2008
# Purpose:  Verify a merge followed by an exlusion works properly.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "exclude15", 0, @ARGV );

# Run both of the CDDs to be merged
system( "$MAKE DIAG=exclude15a onemergerun" ) && die;
system( "$MAKE DIAG=exclude15b onemergerun" ) && die;

# Now perform merge command
&runMergeCommand( "-o exclude15.cdd exclude15a.cdd exclude15b.cdd" );

# Create temporary file that will contain an exclusion message
&runCommand( "echo This line is not needed > exclude15.excl" );

# Perform exclusion
&runExcludeCommand( "-f ../regress/exclude15.cfg < exclude15.excl" );

# Remove temporary exclusion reason file
system( "rm -f exclude15.excl" ) && die;

# Generate reports
&runReportCommand( "-d v -e -x -o exclude15.rptM exclude15.cdd" );
&runReportCommand( "-d v -e -x -i -o exclude15.rptI exclude15.cdd" );

# Perform the file comparison checks
&checkTest( "exclude15", 3, 0 );

exit 0;

