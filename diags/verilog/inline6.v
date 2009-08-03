/*
 Name:        inline6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/03/2009
 Purpose:     Verifying bug fix for bug 2831537.
*/

module main;

reg  [2:0] mem[0:1];
wire [2:0] a, b;

assign a = mem[0],
       b = mem[1];

initial begin
        mem[0] = 3'h0;
        mem[1] = 3'h0;
        #5;
        mem[0] = 3'h5;
        mem[1] = 3'h2;
end

initial begin
`ifdef DUMP
        $dumpfile( "inline6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
