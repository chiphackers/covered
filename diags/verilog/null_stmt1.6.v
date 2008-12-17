/*
 Name:        null_stmt1.6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that null statements after event controls work properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	@(posedge b);
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.6.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
        #20;
        $finish;
end

endmodule
