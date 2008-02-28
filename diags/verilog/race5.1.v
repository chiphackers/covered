/*
 Name:        race5.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/25/2008
 Purpose:     Verifies that embedded racecheck pragmas have the needed effect if the -rP
              option has been specified.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [3:0] a, b;
reg       clk;

// racecheck off
initial begin
	b = 4'h0;
	forever @(posedge clk) begin
 	  b = b + 1;
          a <= ~b;
	end
end
// racecheck on

initial begin
	clk = 1'b0;
	#5;
	clk = 1'b1;
	#5;
	clk = 1'b0;
	#5;
	clk = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "race5.1.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
