/*
 Name:     enum2.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     10/16/2006
 Purpose:  
*/

module main;

enum logic { FOO_0 = 1'bx, FOO_1 = 1'b0 } foo;

initial begin
`ifdef DUMP
        $dumpfile( "enum2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
