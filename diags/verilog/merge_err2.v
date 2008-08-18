/*
 Name:        merge_err2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/04/2008
 Purpose:     See script for details.
*/

module main;

foo f( 1'b0 );

initial begin
`ifdef DUMP
        $dumpfile( "merge_err2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


module foo(
  a
);

input a;

wire b = ~a;

endmodule
