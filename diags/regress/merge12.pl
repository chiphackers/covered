# Name:     merge12.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     07/27/2009
# Purpose:  Verifies that unscored and scored CDD files can be merged without error.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "merge12", 1, @ARGV );

# Create unscored CDD
&runScoreCommand( "-t merge12_dut -i main.dut -v merge12a.v -o merge12a.cdd -y lib" );

# Perform diagnostic running code here
&runCommand( "$MAKE DIAG=merge12b onemergerun" );

# Merge scored CDDs with unscored CDD
&runMergeCommand( "-o merge12.cdd merge12a.cdd merge12b.cdd 2> merge12.err" );
&checkTest( "merge12", 1, 1 );

# Remove the intermediate files
&checkTest( "merge12a", 1, 6 );
&checkTest( "merge12b", 1, 6 );

exit 0;

