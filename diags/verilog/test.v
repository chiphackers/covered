/*
 Name:        generate18.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/20/2009
 Purpose:     
*/

module main;

reg       clock;
reg [1:0] b;

reg  a;

always @(posedge clock)
  casex( b )
    2'b01  :  a <= 1'b1;
    2'b10  :  a <= 1'b0;
    default:  a <= a;
  endcase

wire c = ~a;

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
	b = 2'b01;
	#5;
	// b = 2'b00;
        #10;
        $finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
