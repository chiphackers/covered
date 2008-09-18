/*
 Name:        exclude12.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/18/2008
 Purpose:     See script for details.
*/

module main;

wire a, b, c;
reg  d, e;
        
assign a = d & e;
assign b = d | e;
assign c = d ^ e;

initial begin
`ifdef DUMP
        $dumpfile( "exclude12.3.vcd" );
        $dumpvars( 0, main );
`endif
        d = 1'b0;
        e = 1'b0;
        #5;
        e = 1'b1;
        #10;
        $finish;
end

endmodule
