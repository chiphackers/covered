/*
 Name:     for5.7.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/11/2007
 Purpose:  Verifies local bool declaration in for loops.
*/

module main;

initial begin : foo
	reg [1:0] a;
        reg [1:0] i;
	i = 0;
	#5;
	for( bool i=0; i<1; i++ ) begin : for1
	  a = i;
        end
	i = a;
end

initial begin
`ifndef VPI
        $dumpfile( "for5.7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
