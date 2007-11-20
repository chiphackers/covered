/*
 Name:     for5.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/11/2007
 Purpose:  Verifies parser emits appropriate error when local variable declarations
           are found in for loops when generation is not SV.
*/

module main;

reg [1:0] a, i;

initial begin : foo
	i = 0;
	#5;
	for( integer i=0; i<4; i=i+1 ) begin : for1
	  a = i;
        end
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
