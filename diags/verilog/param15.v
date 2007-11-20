/*
 Name:     param15.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     10/31/2006
 Purpose:  Verifies that implicitly sized parameter value is dependent upon
           the size of its expression.
*/

module main;

parameter foo = 40'hffffffffff,
          bar = 2'h2;

reg [39:0] a;

initial begin
	a = bar;
	#5;
	a = foo;
end

initial begin
`ifndef VPI
        $dumpfile( "param15.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
