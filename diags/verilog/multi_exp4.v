/*
 Name:     multi_exp4.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/08/2006
 Purpose:  
*/

module main;

reg  b, c, d;
wire a = b & (c & d);

initial begin
`ifndef VPI
        $dumpfile( "multi_exp4.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        c = 1'b0;
	d = 1'b1;
	#5;
	b = 1'b1;
	c = 1'b1;
        #10;
        $finish;
end

endmodule
