/*
 Name:        pedge1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that a missed, embedded posedge event wait is output to the report file correctly.
*/

module main;

reg a, b, c, d;

initial begin
	d = 1'b0;
end
 
always @(posedge a or posedge b )
  c <= d;

initial begin
`ifdef DUMP
        $dumpfile( "pedge1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
