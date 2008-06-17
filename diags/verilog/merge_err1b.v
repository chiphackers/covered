/*
 Name:        merge_err1b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/17/2008
 Purpose:     
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg  a, b;
wire c, z;

adder1 addA ( a, b, c, z );

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	a = 1'b1;
	b = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "merge_err1b.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
