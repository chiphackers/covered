/*
 Name:        exclude12.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/18/2008
 Purpose:     See script for details.
*/

module main;

reg [1:0] a[0:3];
reg [1:0] b[0:3];

initial begin
        a[0] = 2'b00;
        #5; 
        a[0] = 2'b11;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude12.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
