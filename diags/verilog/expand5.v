/*
 Name:        expand5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Verify that expansion operation calculates X value when the
              expansion multiplier is unknown.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

parameter TEN = 10;
parameter X   = 32'bx;

reg [79:0] a;

initial begin
	a = 80'h0;
	#5;
	a = {TEN{8'hab}};
	#5;
	a = {X{8'hab}};
	#5;
	$finish;
end

endmodule
