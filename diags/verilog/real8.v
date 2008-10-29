/*
 Name:        real8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/29/2008
 Purpose:     Verify that assigning a real value to x results in the value being
              assigned to 0.0.
*/

module main;

real a;
reg  b;

initial begin
	a = 123.456;
	b = 1'b0;
	#5;
	a = 64'hxxxxxxxxxxxxxxxx;
	b = (a == 0.0);
end

initial begin
`ifdef DUMP
        $dumpfile( "real8.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
