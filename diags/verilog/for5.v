/*
 Name:     for5.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     09/10/2007
 Purpose:  Verifies local variable declarations in for loops.
*/

module main;

reg [1:0] i;

initial begin : foo
	reg [1:0] a;
	i = 0;
	#5;
	for( int i=0; i<4; i++ )
	  a = i;
	i = a;
end

initial begin
`ifndef VPI
        $dumpfile( "for5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
