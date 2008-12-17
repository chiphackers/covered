/*
 Name:        null_stmt1.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	if( a )
	  b = 1'b1;
	else
	  ;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
