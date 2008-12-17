/*
 Name:        null_stmt1.4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that null statements within $root tasks work properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

task foo;
reg a;
;
endtask

//---------------------------------------------------

module main;

reg a;

initial begin
	a = 1'b0;
	$root.foo;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
