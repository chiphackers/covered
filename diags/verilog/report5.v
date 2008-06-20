/*
 Name:        report5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/20/2008
 Purpose:     See script for details.
*/

module main;

reg a;

foo f();

initial begin
`ifdef DUMP
        $dumpfile( "report5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


// Have module which has no coverage.
module foo;
endmodule
