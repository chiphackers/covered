/*
 Name:        generate15.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/04/2009
 Purpose:     Recreates bug 2565447.  The parser emits an error with the '&&' operator in the if statement.
*/

module main;

parameter FOO1 = 0;
parameter FOO2 = 1;

reg  [3:0] a;
reg  [3:0] b;
wire [4:0] c;

generate
   if ((FOO1 == 0) && (FOO2 == 0)) begin : add
       assign c = a + b;
   end else begin
       assign c = a - b;
   end
endgenerate

initial begin
	a = 4'h8;
	b = 4'h4;
        #5;
	b = 4'h3; 
end

initial begin
`ifdef DUMP
        $dumpfile( "generate15.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
