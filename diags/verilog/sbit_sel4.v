/*
 Name:        sbit_sel4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/04/2008
 Purpose:     Verifies that a single-bit select assignment to a vector
              bit-fills correctly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a;
reg [3:0]  b;

initial begin
	a = 32'hffffffff;
	b = 4'bzx10;
	#5;
	a = b[0];
end

initial begin
`ifdef DUMP
        $dumpfile( "sbit_sel4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
