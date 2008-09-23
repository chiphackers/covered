/*
 Name:        delay3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/23/2008
 Purpose:     Verify a corner case where a non-blocking assignment with a simple
              delay preceding an unsized value exists.
*/

module main;

reg [7:0] a;
reg       b;

always @(posedge b) a <= #1 'b0;

initial begin
`ifdef DUMP
        $dumpfile( "delay3.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	#5;
	b = 1'b1;
        #10;
        $finish;
end

endmodule
