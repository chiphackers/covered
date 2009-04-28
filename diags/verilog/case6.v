/*
 Name:        case6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/09/2009
 Purpose:     Verifies that comma-separated case items work properly
*/

module main;

reg [1:0] a;
reg [3:0] b;

always @* begin
  b = 4'h0;
  #1;
  case( a )
    2'b00,
    2'b01 :  b[a] = 1'b1;
    2'b10,
    2'b11 :  b[a] = 1'b1;
  endcase
end

initial begin
`ifdef DUMP
        $dumpfile( "case6.vcd" );
        $dumpvars( 0, main );
`endif
	a = 2'b00;
	#5;
	a = 2'b10;
	#5;
	a = 2'b01;
	#5;
	a = 2'b11;
        #10;
        $finish;
end

endmodule
