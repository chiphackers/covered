/*
 Name:        mbit_sel9.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/05/2008
 Purpose:     Verify that multi-bit select assignment to larger value
              is padded correctly.
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
	a = b[1:0];
end

initial begin
`ifdef DUMP
        $dumpfile( "mbit_sel9.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
