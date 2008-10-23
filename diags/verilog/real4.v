/*
 Name:        real4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies that the $shortrealtobits and $bitstoshortreal system functions work properly.
*/

module main;

shortreal  a, b;
reg [31:0] c;
reg        d;

initial begin
        a = 123.456;
        d = 1'b0;
        #5;
        c = $shortrealtobits( a );
        b = $bitstoshortreal( c );
        d = (a == b);
end

initial begin
`ifdef DUMP
        $dumpfile( "real4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
