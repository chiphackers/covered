require "../verilog/regress_subs.pl";

$OUTPUT = "merge_err1.output";

# Create a temporary file with read-only permissions
system( "touch $OUTPUT; chmod 400 merge_err1.output" ) && die;

system( "make DIAG=merge_err1a diagrun" ) && die;
system( "make DIAG=merge_err1b diagrun" ) && die;
&runCommand( "$COVERED merge -o $OUTPUT merge_err1a.cdd merge_err1b.cdd 2> merge_err1.err | ./check_mem" );
system( "cat merge_err1.err" ) && die;

# Perform diagnostic check
&checkTest( "merge_err1", 1, 1 );

# Remove temporary file
system( "rm -f $OUTPUT" ) && die;

exit 0;
