/*
 Name:     merge3b.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     10/16/2006
 Purpose:  Verify that merging two designs with full hierarchy can
           be successfully done (proving fix for bug 1578251).
*/

module main;

reg  b, c;

wire a = b & c;

initial begin
	b = 1'b1;
	c = 1'b1;
	#5;
        c = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "merge3b.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
