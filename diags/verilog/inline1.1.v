/*
 Name:        inline1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/23/2009
 Purpose:     Verifies that an always block followed by an assign is generated
              properly.
*/

module main;

reg       a, c, d;
reg [1:0] b;
wire      x;

always @(c or d)
  if( a ) 
    b = {d, c};
  else if( !a )
    b = {c, d};assign x = ~d;

initial begin
`ifdef DUMP
        $dumpfile( "inline1.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	c = 1'b0;
	d = 1'b1;
	#5;
	d = 1'b0;
        #10;
        $finish;
end

endmodule
