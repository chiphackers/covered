/*
 Name:        null_stmt1.7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that null statements after wait statement work properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	wait( b == 1'b0 );
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.7.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        #10;
        $finish;
end

endmodule
