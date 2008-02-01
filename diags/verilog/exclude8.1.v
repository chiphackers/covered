/*
 Name:        exclude7.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verifies that expressions/statements can be included when the coverage pragma
              is ignored.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b, c;

initial begin
	a = 1'b0;
	#5;
	a = b | c;
	#5;
	// coverage off
	a = b & c;
	// coverage on
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude8.1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b1;
        #20;
        $finish;
end

endmodule
