/*
 Name:        race6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/27/2008
 Purpose:     Verifies that for loop control assignments and non-blocking operators
              can exist in the same always block and race condition checking will not
              eliminate the block from coverage consideration.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg       clock;
reg [3:0] a;
reg [7:0] b;
integer   i;

always @(posedge clock)
  for( i=0; i<4; i=i+1 )
    a[i] <= b[i];

initial begin
`ifdef DUMP
        $dumpfile( "race6.vcd" );
        $dumpvars( 0, main );
`endif
        b = 8'h0;
	clock = 1'b0;
	#5;
	clock = 1'b1;
	#5;
	clock = 1'b0;
        b = 8'h69;
	#5;
	clock = 1'b1;
        #10;
        $finish;
end

endmodule
