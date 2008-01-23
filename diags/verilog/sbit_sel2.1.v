/*
 Name:     sbit_sel2.1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     01/23/2008
 Purpose:  Verify that we can assign to a single bit select
           that is out-of-bounds without error or side-effect.
*/

module main;

reg [4:1] a;
reg       b;

initial begin
	a    = 4'hf;
        b    = 1'b1;
	a[0] = 1'b0;    // This should be the equivalent of a nop
        b    = a[0];    // b should be assigned X
end

initial begin
`ifdef DUMP
        $dumpfile( "sbit_sel2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
