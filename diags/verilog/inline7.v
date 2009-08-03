/*
 Name:        inline7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/03/2009
 Purpose:     Verifies bug fix for bug 2831612.
*/

module main;

reg       a, b, c;
reg [1:0] d;
reg [2:0] mem[0:1];

always @* begin : foobar
  if( a ) begin
    if( b && c )
      d = 2'b00;
    else if( mem[0] )
      d = 2'b01;
    else
      d = 2'b10;
  end
end

initial begin
	mem[0] = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "inline7.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b0;
	c = 1'b0;
	#5;
	a = 1'b1;
        #10;
        $finish;
end

endmodule
