/*
 Name:        cond3.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Perform a conditional that contains an unsigned binary, octal and hexidecimal value
              that does not achieve full coverage.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [15:0] a;
reg        b, c;

initial begin
	a = 0;
	b = 1'b1;
	c = 1'b1;
	#5;
	a = b ? (c ? 'b1 : 'o2) : 'h3;
end

initial begin
`ifdef DUMP
        $dumpfile( "cond3.3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
