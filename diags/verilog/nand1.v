/*
 Name:        nand1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that a missed NAND operation can be output correctly.
*/

module main;

wire a;
reg  b, c;

assign a = b ~& c;

initial begin
`ifdef DUMP
        $dumpfile( "nand1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b0;
	#5;
	c = 1'b1;
        #10;
        $finish;
end

endmodule
