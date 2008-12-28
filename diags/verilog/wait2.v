/*
 Name:        wait2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that statements immediately after wait statements work properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

initial begin
	b = 1'b0;
	a = 1'b0;
	#5;
	wait( b == 1'b0 ) b = 1'b1;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "wait2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
