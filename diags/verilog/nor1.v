/*
 Name:        nor1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that a NOR operation that has not been fully hit is output correctly to
              the report file.
*/

module main;

wire a;
reg  b, c;

assign a = b ~| c;

initial begin
`ifdef DUMP
        $dumpfile( "nor1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	c = 1'b0;
	#5;
	c = 1'b1;
        #10;
        $finish;
end

endmodule
