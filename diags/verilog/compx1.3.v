/*
 Name:        compx1.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that greater-than-or-equal comparison works properly when one comparison
              value contains an X value.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [63:0] a, b;
reg        c, d, e;

initial begin
	a = 0;
	b = 1;
	c = 1'b0;
	d = 1'b0;
	e = 1'b0;
	#5;
	if( b >= a )
	  c = 1'b1;
	#5;
	a[2] = 1'bx;
	if( b >= a )
          d = 1'b1;
	#5;
	b[63] = 1'b1;
	if( b >= a )
          e = 1'b1;
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "compx1.3.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
