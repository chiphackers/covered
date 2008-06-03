/*
 Name:        null_stmt2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/02/2008
 Purpose:     Verifies that a null statement can exist in a statement block.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
	;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
