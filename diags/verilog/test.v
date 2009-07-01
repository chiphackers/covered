/*
 Name:        generate18.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/20/2009
 Purpose:     
*/

module main;

parameter FOO = 128'hffffffff_ffffffff_ffffffff_ffffffff;

initial begin
	$display( "foo: %h", FOO );
end

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
