# Name:     report8.pl
# Author:   Trevor Williams  (phase1geo@gmail.com)
# Date:     06/20/2008
# Purpose:  Verifies that if output report file is not specified that the output
#           is directed to standard output.

require "../verilog/regress_subs.pl";

# Initialize the diagnostic environment
&initialize( "report8", 0, @ARGV );

# Simulate and get coverage information
if( $SIMULATOR eq "IV" ) { 
  system( "iverilog -DDUMP report8.v; ./a.out" ) && die;
} elsif( $SIMULATOR eq "CVER" ) {
  system( "cver -q +define+DUMP report8.v" ) && die;
} elsif( $SIMULATOR eq "VCS" ) {
  system( "vcs +define+DUMP report8.v; ./simv" ) && die;
} elsif( $SIMULATOR eq "VERIWELL" ) {
  system( "veriwell +define+DUMP report8.v" ) && die;
}

$check_mem_cmd = $CHECK_MEM_CMD;

&runScoreCommand( "-t main -vcd report8.vcd -v report8.v -o report8.cdd" );
$CHECK_MEM_CMD = "$check_mem_cmd > report8.rptM";
&runReportCommand( "-d v -m ltcfam report8.cdd" );
$CHECK_MEM_CMD = "$check_mem_cmd > report8.rptI";
&runReportCommand( "-d v -m ltcfam -i report8.cdd" );

# Perform the file comparison checks
if( $DUMPTYPE eq "VCD" ) { 
  &checkTest( "report8", 1, 0 );
} else {
  &checkTest( "report8", 1, 5 );
}

exit 0;

