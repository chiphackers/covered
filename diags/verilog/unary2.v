/*
 Name:        unary2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that a unary AND/NAND operations on vectors greater than 32-bits works properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [79:0] a;
reg        b, c;

initial begin
	a = 80'hffff_0000ffff_0000ffff;
	b = 1'b0;
        c = 1'b0;
	#5;
	b = &a;
	c = ~&a;
	#5;
	a = 80'hffff_ffffffff_ffffffff;
	b = &a;
	c = ~&a;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "unary2.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
