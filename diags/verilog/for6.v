/*
 Name:     for6.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/11/2007
 Purpose:  Verifies multiple local declarations in for loops.
*/

module main;

shortint k, m;

initial begin : foo
	reg [1:0] a;
        reg [1:0] i;
	i = 0;
	#5;
	for( int i=0, j=0, k=0, char l=0, longint m=0; i<4; i++ ) begin : for1
	  a = i;
        end
	i = a;
end

initial begin
`ifdef DUMP
        $dumpfile( "for6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
