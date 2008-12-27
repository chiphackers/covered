/*
 Name:        sbit_sel3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/29/2008
 Purpose:     
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a, b;

initial begin
	a = 0;
	b = 1;
	#5;
	a[b+1] = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "sbit_sel3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
