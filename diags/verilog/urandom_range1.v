/*
 Name:        urandom_range1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/04/2008
 Purpose:     Verifies that the $urandom_range system task call works when only one value is supplied.
*/

module main;

integer i, a;

initial begin
	for( i=0; i<4; i=i+1 ) begin
	  a = $urandom_range( 7 );
          #5;
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom_range1.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
