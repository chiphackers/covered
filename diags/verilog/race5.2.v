/*
 Name:        race5.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/25/2008
 Purpose:     Verifies that embedded racecheck pragmas can be renamed by the user.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [3:0] a, b;
reg       clk;

// rcc off
initial begin
	b = 4'h0;
	forever @(posedge clk) begin
	  b = b + 1;
          a <= ~b;
	end
end
// rcc on

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
        $dumpfile( "race5.2.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
