/*
 Name:        elsif3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/03/2009
 Purpose:     Verifies that code immediately following an `elsif preprocessor
              directive works properly.
*/

module main;

reg a;

`ifdef FOOBAR
`elsif RUNTEST initial begin a = 1'b0; #5; a = 1'b1; end
`endif

initial begin
`ifdef DUMP
        $dumpfile( "elsif3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
