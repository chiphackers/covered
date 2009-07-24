/*
 Name:        inline2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/24/2009
 Purpose:     Verifies that an intermediate signal is not instantiated twice.
*/

module main;

reg       clock;
reg [1:0] a[0:1];
reg       wen;
reg [1:0] c;

always @(posedge clock) begin
  a[0] <= wen ? c : a[0];
end

initial begin
`ifdef DUMP
        $dumpfile( "inline2.vcd" );
        $dumpvars( 0, main );
`endif
	c   <= 2'b01;
	wen <= 1'b0;
	@(posedge clock);
	wen <= 1'b1;
	@(posedge clock);
	@(posedge clock);
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
