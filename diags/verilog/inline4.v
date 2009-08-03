/*
 Name:        inline4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/03/2009
 Purpose:     Verifies that whitespace following if block is handled correctly.
*/

module main;

reg clock;
reg a, b, c;

always @(posedge clock) begin
  if( c ) begin
    if( a ) begin
      b <= ~a;
    end
  end
  c <= a;
end

initial begin
`ifdef DUMP
        $dumpfile( "inline4.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	#10;
	a = 1'b1;
        #10;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
