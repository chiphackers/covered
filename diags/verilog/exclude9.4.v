/*
 Name:        exclude9.4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verify that 'coverage off' is only applied to current file.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

foo_module f( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

// coverage off

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

endmodule
