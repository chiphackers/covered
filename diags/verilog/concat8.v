/*
 Name:        concat8.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        11/07/2008
 Purpose:     Verifies that concatenations with impossible values are not output to
              report file.
*/

module main;

wire [2:0] a, b, c, d, e, f, g, h, i, j, k;
reg        z;

assign a = {1'b0,1'b0,1'b0};
assign b = {1'b0,1'b1,1'b0};
assign c = {1'b0,1'b0,z};
assign d = {1'b0,1'b1,z};
assign e = {1'b1,1'b0,z};
assign f = {1'b0,z,1'b0};
assign g = {1'b0,z,1'b1};
assign h = {1'b1,z,1'b0};
assign i = {z,1'b0,1'b0};
assign j = {z,1'b0,1'b1};
assign k = {z,1'b1,1'b0};

initial begin
`ifdef DUMP
        $dumpfile( "concat8.vcd" );
        $dumpvars( 0, main );
`endif
	z = 1'b0;
        #10;
        $finish;
end

endmodule
