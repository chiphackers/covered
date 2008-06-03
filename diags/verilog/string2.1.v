/*
 Name:        string2.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/02/2008
 Purpose:     Verifies that part selection of string expression gets the correct information.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [(10*8)-1:0] a;
reg [7:0]        b;

initial begin
	a = 80'h0;
        b = 8'h0;
	#5;
	a = "foo";
	b = a[7:0];
end

initial begin
`ifdef DUMP
        $dumpfile( "string2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
