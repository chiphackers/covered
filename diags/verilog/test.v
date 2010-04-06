/*
 Name:        generate18.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/20/2009
 Purpose:     
*/

module main;

reg  clock;
reg  a, reset;
wire b;

always @(posedge clock) a <= reset ? 1'b0 : b;

assign b = ~a;

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
	reset = 1'b1;
	#5;
	reset = 1'b0;
        #20;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
