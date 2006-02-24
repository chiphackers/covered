module main;

reg  b;
wire a = b;

initial begin
	$dumpfile( "err2.0.vcd" );
	$dumpvars( 0, main );
	b = 1'b1;
	#10;
	$finish;
end

endmodule

/* HEADER
GROUPS err2.0 all iv vcs vcd lxt
SIM    err2.0 all iv vcd  : iverilog err2.0.v; ./a.out                             : err2.0.vcd
SIM    err2.0 all iv lxt  : iverilog err2.0.v; ./a.out -lxt2; mv err2.0.vcd err2.0.lxt : err2.0.lxt
SIM    err2.0 all vcs vcd : vcs err2.0.v; ./simv                                   : err2.0.vcd
SCORE  err2.0.vcd     : -t main -vcd err2.0.vcd -o ./tmp/err2.0.cdd -v err2.0.v >& err2.0.err : err2.0.err : 1
SCORE  err2.0.lxt     : -t main -lxt err2.0.lxt -o ./tmp/err2.0.cdd -v err2.0.v >& err2.0.err : err2.0.err : 1
*/

/* OUTPUT err2.0.err
ERROR!  Could not open ./tmp/err2.0.cdd for writing
ERROR!  Unable to write database file
*/
