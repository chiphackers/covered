/*
 Name:        urandom1.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/04/2008
 Purpose:     Verifies that the seed value to a $urandom system task call can come from a memory element.
*/

module main;

reg [31:0] seeds[0:1];
integer    i, a;

initial begin
	seeds[0] = 1;
	seeds[1] = 2;
	#5;
        for( i=0; i<4; i=i+1 ) begin
          a = $urandom( seeds[0] );
	  $display( "a: %d", a );
	  #5;
	end
end

initial begin
`ifdef DUMP
        $dumpfile( "urandom1.2.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
