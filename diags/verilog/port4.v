/*
 Name:     port4.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     12/31/2007
 Purpose:  Verify that function input port is sized correctly.
*/

module main;

integer a;
wire [31:0] b;

foo foo_mod(a,b);

initial begin
`ifdef DUMP
        $dumpfile( "port4.vcd" );
        $dumpvars( 0, main );
`endif
	a = 0;
	#5;
	a = 10;
        #5;
        $finish;
end

endmodule


module foo(a,b);

input a;
output [31:0] b;

wire [31:0] a;

assign b = a;

endmodule
