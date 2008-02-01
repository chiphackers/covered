/*
 Name:        exclude9.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verify that coverage exclusion can work over multiple modules within same file.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

foobar f();

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

// coverage off

endmodule


module foobar;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

endmodule

// coverage on
