/*
 Name:        generate18.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/20/2009
 Purpose:     
*/

module main;

reg a, b, c, d;

initial begin
	a = 1'b0;
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
	#5;
	if(a&&(b||c))
	  d = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
