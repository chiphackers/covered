/*
 Name:        slist6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2009
 Purpose:     Verifies that memories within implicit sensitivity list works properly.
*/

module main;

reg a, mem[1:0];

always @*
  a = mem[0];

initial begin
	mem[0] = 1'b1;
        #5;
        mem[0] = 1'b0;
end

initial begin
`ifdef DUMP
        $dumpfile( "slist6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
