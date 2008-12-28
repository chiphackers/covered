/*
 Name:        timescale4.11.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/24/2008
 Purpose:     Verifies that real delays work properly when timescale is 100 s / 1 ns
*/

`timescale 100 s / 1 ns

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
        $dumpfile( "timescale4.11.vcd" );
        $dumpvars( 0, main );
`endif
        #(10);
        $finish;
end

endmodule

`timescale 1 ns / 1 ns

module inner(
  input b
);

reg a;

initial begin
        a = 1'b0;
        @(posedge b);
        a = ($time == 64'd212345678999);
end

endmodule

