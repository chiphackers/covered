/*
 Name:        exclude9.5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verify that any extra inforamtion after the 'on' or 'off' value is ignored.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

// coverage off a;sldfkja;sdfkjasdf
reg a;
// coverage on assdffdsaasdf

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
