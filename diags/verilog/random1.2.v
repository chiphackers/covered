/*
 Name:        random1.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/02/2008
 Purpose:     Verifies that the seed value to a $random system task call can come from a memory element.
*/

module main;

reg [31:0] seeds[0:1];
integer    i, a;

initial begin
	seeds[0] = 1;
	seeds[1] = 2;
	#5;
        for( i=0; i<4; i=i+1 ) begin
          a = $random( seeds[0] );
	  #5;
	end
end

initial begin
`ifdef DUMP
        $dumpfile( "random1.2.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
