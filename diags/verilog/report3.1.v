/*
 Name:        report3.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/18/2008
 Purpose:     See script for details.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#2;
	a = 1'b1;
	#2;
	a = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "report3.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
