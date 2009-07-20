/*
 Name:        assign5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/18/2009
 Purpose:     Verify that comma-separated assignments work properly.
*/

module main;

reg  e, f;
wire a, b, c, d;

assign a = e ~& f;
assign b = e & f,
       c = e | f,
       d = e ^ f;

initial begin
`ifdef DUMP
        $dumpfile( "assign5.vcd" );
        $dumpvars( 0, main );
`endif
	e = 1'b0;
	f = 1'b0;
	#5;
	f = 1'b1;
	#5;
	e = 1'b1;
        #5;
        $finish;
end

endmodule
