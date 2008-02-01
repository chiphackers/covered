/*
 Name:        exclude9.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verify that exclusion pragma works when a multi-line comment is used
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

/* coverage
   off
 */
reg a;
/* coverage
   on
 */

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
