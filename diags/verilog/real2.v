/*
 Name:        real2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/16/2008
 Purpose:     Verifies that a real value can be assigned to a register.
*/

module main;

reg [31:0] a;

initial begin
	a = 0;
	#5;
	a = (1.1 + 3.6);
end

initial begin
`ifdef DUMP
        $dumpfile( "real2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
