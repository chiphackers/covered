/*
 Name:     timescale1.19.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/25/2006
 Purpose:  Verifies 100 s / 1 fs timescale setting.
*/

`timescale 100 s / 1 fs

module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
`ifndef VPI
        $dumpfile( "timescale1.19.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
