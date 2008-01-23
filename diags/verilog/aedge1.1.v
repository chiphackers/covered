/*
 Name:     aedge1.1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     01/18/2008
 Purpose:  Verifies that a multi-bit anyedge works properly.
*/

module main;

reg [3:0]  a;
reg [31:0] b;

initial begin
	#1;
	b = 1;
	forever @(a) b = b << 1;
end

initial begin
	a = 4'h0;
	#5;
	a = 4'h1;
	#5;
	a = 4'h1;
	#5;
	a = 4'h2;
	#5;
	a = 4'h2;
	#5;
	a = 4'h4;
	#5;
	a = 4'h8;
end

initial begin
`ifdef DUMP
        $dumpfile( "aedge1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
