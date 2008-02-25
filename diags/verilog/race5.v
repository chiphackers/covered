/*
 Name:        race5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/25/2008
 Purpose:     Verifies that embedded racecheck pragmas have no effect if the -rP
              option has not been specified.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [3:0] a, b;
reg       clk;
integer   i;

// racecheck off
always @(posedge clk) begin
  for( i=0; i<4; i=i+1 )
    a[i] <= ~b[i];
end
// racecheck on

initial begin
	b   = 4'b0000;
	clk = 1'b0;
	#5;
	clk = 1'b1;
	#5;
	b   = 4'b1001;
	clk = 1'b0;
	#5;
	clk = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "race5.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
