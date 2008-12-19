/*
 Name:        null_stmt1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that null statement in the true portion of an if
              statement works properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
	a = 1'b1;
	if( a )
          ;
	#5;
	a = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
