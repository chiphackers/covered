/*
 Name:        null_stmt2.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/02/2008
 Purpose:     Verifies that a null statement can exist in a statement block.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
	;a = 1'b0;;
	#5;
	;
	a = ~a;;;;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
