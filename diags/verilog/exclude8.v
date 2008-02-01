/*
 Name:        exclude7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Does not exclude a signal from coverage consideration via coverage pragma
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

// coverage off
reg a;
// coverage on

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude8.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
