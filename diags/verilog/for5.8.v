/*
 Name:     for5.8.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/11/2007
 Purpose:  Verifies local bit declaration in for loops.
*/

module main;

initial begin : foo
	reg [1:0] a;
        reg [1:0] i;
	i = 0;
	#5;
	for( bit [2:0] i=0; i<4; i++ ) begin : for1
	  a = i;
        end
	i = a;
end

initial begin
`ifndef VPI
        $dumpfile( "for5.8.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
