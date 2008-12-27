/*
 Name:        random1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/02/2008
 Purpose:     Verify that a seeded $random call works properly.
*/

module main;

integer i, a, b;

initial begin
        b = 1;
        for( i=0; i<4; i=i+1 ) begin
          #5;
          a = $random( b );
	end
end

initial begin
`ifdef DUMP
        $dumpfile( "random1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
