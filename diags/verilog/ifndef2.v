/*
 Name:        ifndef2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/03/2009
 Purpose:     
*/

module main;

reg a;

`ifndef FOOBAR initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end
`endif

initial begin
`ifdef DUMP
        $dumpfile( "ifndef2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
