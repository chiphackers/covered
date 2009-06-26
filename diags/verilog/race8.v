/*
 Name:        race8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/25/2009
 Purpose:     Reproduces race condition bug 2812495.
*/

module main;

reg a;
reg clock;

foo f( clock, a );

initial begin
`ifdef DUMP
        $dumpfile( "race8.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	#5;
	a = 1'b1;
        #5;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #2 clock = ~clock;
end

endmodule

//-----------------------------------------------

module foo(
  input clock,
  input a
);

reg  b;
wire c;

always @(posedge clock) begin
  b = ~a;
end

assign c = ~b;

endmodule
