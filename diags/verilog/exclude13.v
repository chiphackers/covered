/*
 Name:        exclude13.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/18/2008
 Purpose:     See script for details.
*/

module main;

reg a;

initial begin
        a = 1'b0;
        #100;
        a = 1'b1;
end     

initial begin
`ifdef DUMP
        $dumpfile( "exclude13.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
