/*
 Name:        err5.4b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/25/2008
 Purpose:     Verify that a vector merge operation with an error in the merged CDD file is
              output and handled correctly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

wire a;
reg  b, c;

dut_and dand( a, b, c );

initial begin
`ifdef DUMP
        $dumpfile( "err5.4b.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b1;
	c = 1'b1;
        #10;
        $finish;
end

endmodule
