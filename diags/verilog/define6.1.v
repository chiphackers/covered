/*
 Name:        define6.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/30/2008
 Purpose:     Verify that multi-line define that does not contain the usual spaces is handled
              correctly
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

`define FOO \
begin\
end

module main;

reg a;

initial begin
	a = 1'b0;
	`FOO
	if( a )
  	  a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "define6.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
