/*
 Name:     aedge1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     01/18/2008
 Purpose:  Verifies that an anyedge works properly.
*/

module main;

reg a;
reg [31:0] b;

initial begin
	#1;
	b = 1;
	forever @(a) b = b << 1;
end

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
	#5;
	a = 1'b1;
	#5;
	a = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "aedge1.vcd" );
        $dumpvars( 0, main );
`endif
        #25;
        $finish;
end

endmodule
