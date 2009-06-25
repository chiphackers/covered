/*
 Name:        assign4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/25/2009
 Purpose:     Verify that assign statements following a procedural
              block gets output correctly by the inlined code generator.
*/

module main;

reg  a, b;
wire c;
reg  clock;

always @(posedge clock)
  a <= b;

assign c = ~a;

initial begin
`ifdef DUMP
        $dumpfile( "assign4.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
        #10;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
