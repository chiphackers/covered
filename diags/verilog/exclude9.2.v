/*
 Name:        exclude9.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verifies that a "coverage" pragma without a value does nothing
              and that a "coverage on" does not cause the exclude_mode to
              go to a negative value.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

// coverage
reg a;
// coverage on
// coverage off
reg b;
// coverage on

initial begin
	a = 1'b0;
	b = 1'b1;
	#5;
	a = 1'b1;
	b = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
