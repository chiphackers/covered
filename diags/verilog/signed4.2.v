/*
 Name:     signed4.2.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/06/2006
 Purpose:  
*/

module main;

wire signed [1:0] a;

initial begin
`ifdef DUMP
        $dumpfile( "signed4.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
