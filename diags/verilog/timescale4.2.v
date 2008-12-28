/*
 Name:        timescale4.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/24/2008
 Purpose:     Verifies that real delays work properly when timescale is 100 s / 1 s
*/

`timescale 100 s / 1 s

module main;

reg a, b;

inner foo( b );

initial begin
	a = 1'b0;
	b = 1'b0;
	#(2.123_456_789_987_654);
	b = 1'b1;
	a = ($time == 2);
end

initial begin
`ifdef DUMP
        $dumpfile( "timescale4.2.vcd" );
        $dumpvars( 0, main );
`endif
        #(10);
        $finish;
end

endmodule

`timescale 1 s / 1 s

module inner(
  input b
);

reg a;

initial begin
	a = 1'b0;
	@(posedge b);
	a = ($time == 212);
end

endmodule
