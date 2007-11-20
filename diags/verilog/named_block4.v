/*
 Name:     named_block4.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     08/06/2007
 Purpose:  Used to verify fix for bug 1621096.
*/

module main;

reg a, b;

always
  begin : foo
    #5;
    a = b;
  end

initial begin
`ifndef VPI
        $dumpfile( "named_block4.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	#6;
	b = 1'b1;
	#5;
	b = 1'b0;
        #10;
        $finish;
end

endmodule
