/*
 Name:        not1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verifies that an X value is calculated for a logical not (!) when
              an X value exists in its value.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [39:0] a;
reg        b, c;

initial begin
	a = 40'h0;
        b = 1'b0;
        c = 1'b0;
	#5;
	if( !a )
	  b = 1'b1;
	#5;
	a[10] = 1'bz;
	if( !a )
	  c = 1'b1;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "not1.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
