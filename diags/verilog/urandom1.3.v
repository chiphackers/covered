/*
 Name:        urandom1.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/04/2008
 Purpose:     Verifies that the $urandom call can be used with an mbit_sel parameter.
*/

module main;

reg [63:0] b;
integer    i, a;

initial begin
	b[63:32] = 1'b1;
	#5;
	for( i=0; i<4; i=i+1 ) begin
          a = $urandom( b[63:32] );
	  #5;
	end
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom1.3.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
