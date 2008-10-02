/*
 Name:        random1.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/02/2008
 Purpose:     Verifies that the $random call can be used with an mbit_sel parameter.
*/

module main;

reg [63:0] b;
integer    i, a;

initial begin
	b[63:32] = 1'b1;
	#5;
	for( i=0; i<4; i=i+1 ) begin
          a = $random( b[63:32] );
	  #5;
	end
end

initial begin
`ifdef DUMP
        $dumpfile( "random1.3.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
