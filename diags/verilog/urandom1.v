/*
 Name:        urandom1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/03/2008
 Purpose:     Verifies that the $urandom function works properly.
*/

module main;

integer i, a;

initial begin
	for( i=0; i<4; i=i+1 ) begin
          a = $urandom;
          #5;
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom1.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
