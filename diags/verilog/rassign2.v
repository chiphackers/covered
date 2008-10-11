/*
 Name:        rassign2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/10/2008
 Purpose:     Verifies that an initialized register that is assigned within a block that gets
              removed from coverage consideration gets its value from the dumpfile.
*/

module main;

reg a = 1'b0;

initial begin
	a = #1.0 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "rassign2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
