/*
 Name:        null_stmt1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that null statement in the true block of an if..then..else
              block works properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

initial begin
	a = 1'b1;
	b = 1'b0;
	#5;
	if( a )
	  ;
	else
	  b = 1'b1;
	a = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
