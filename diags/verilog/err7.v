/*
 Name:        err7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/07/2008
 Purpose:     Verify that generating a report on a non-scored CDD file is
              handled appropriately.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
	#5;
	$finish;
end

endmodule
