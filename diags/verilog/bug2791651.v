/*
 Name:        bug2791651.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/14/2009
 Purpose:     Verifies that we don't get a segmentation fault with the given code.
*/

module main;

begin
end

initial begin
`ifdef DUMP
        $dumpfile( "bug2791651.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
