/*
 Name:     enum2.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     10/16/2006
 Purpose:  
*/

module main;

enum logic { FOO_0 = 1'bx, FOO_1 } foo;

initial begin
`ifndef VPI
        $dumpfile( "enum2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
