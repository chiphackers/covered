/*
 Name:        score_env1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/23/2008
 Purpose:     
*/

module main;

reg  a, b;
wire c, d;

adder1 adder( a, b, c, d );

initial begin
`ifdef DUMP
        $dumpfile( "score_env1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'b0;
	b = 1'b1;
        #10;
        $finish;
end

endmodule
