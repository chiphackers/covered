/*
 Name:        merge5b.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/01/2008
 Purpose:     
*/

module main;

reg [127:0] a, b, c;
reg [  2:0] d;

ranker r( a, b, c, d );

initial begin
`ifdef DUMP
        $dumpfile( "merge5b.vcd" );
        $dumpvars( 0, main );
`endif
	a = 128'h0;
	b = 128'h0;
	c = 128'h0;
	d = 3'h0;
	#5;
	a = 128'h33333333_22222222_11111111_00000000;
        b = 128'h77777777_66666666_55555555_44444444;
	c = 128'hbbbbbbbb_aaaaaaaa_99999999_88888888;
	d = 3'h2;
        #10;
        $finish;
end

endmodule
