# Name:     merge12.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     07/27/2009
# Purpose:  Verifies that unscored CDD files merged with scored CDD files causes the
#           unscored CDD file to be discarded.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge12", 0, @ARGV );

if( $DUMPTYPE eq "VCD" ) {
  $check_code = 0;
} else {
  $check_code = 5;
}

# Create unscored CDD
&runScoreCommand( "-t merge12_dut -i main.dut -v merge12a.v -o merge12a.cdd -y lib" );

# Perform diagnostic running code here
&runCommand( "$MAKE DIAG=merge12b onemergerun" );

# Merge scored CDDs with unscored CDD
&runMergeCommand( "-o merge12.cdd merge12a.cdd merge12b.cdd" );
&runReportCommand( "-d v -o merge12.rptM merge12.cdd" );
&runReportCommand( "-d v -i -o merge12.rptI merge12.cdd" );
&checkTest( "merge12", 1, $check_code );

# Remove the intermediate files
&checkTest( "merge12a", 1, 6 );
&checkTest( "merge12b", 1, 6 );

exit 0;

