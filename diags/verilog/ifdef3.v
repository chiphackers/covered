/*
 Name:        ifdef3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/03/2009
 Purpose:     Verify that legal Verilog code can follow an `ifdef preprocessor directive.
*/

module main;

reg a;

`ifdef RUNTEST initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end
`endif

initial begin
`ifdef DUMP
        $dumpfile( "ifdef3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
