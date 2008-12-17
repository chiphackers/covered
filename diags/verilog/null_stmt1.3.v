/*
 Name:        null_stmt1.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that null statement within a task works correctly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
	a = 1'b0;
	foo;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

task foo;
reg a;
;
endtask

endmodule
