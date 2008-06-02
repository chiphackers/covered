/*
 Name:        string2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/02/2008
 Purpose:     Verify that empty string is handled correctly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [40*8-1:0] a;

initial begin
	a = "";
	#5;
	a = "FOO";
end

initial begin
`ifdef DUMP
        $dumpfile( "string2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
