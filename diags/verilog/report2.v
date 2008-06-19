/*
 Name:        report2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/18/2008
 Purpose:     See the script for details.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
	#10;
	a = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "report2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
