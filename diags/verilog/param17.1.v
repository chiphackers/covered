/*
 Name:        param17.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/31/2008
 Purpose:     Verify that parameter sized port that is also declared works properly even when
              another wire is declared on the same line as the port.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [3:0] a;

foo f( a );

initial begin
`ifdef DUMP
        $dumpfile( "param17.1.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'b0;
	#5;
	a = 4'b1;
        #10;
        $finish;
end

endmodule


module foo(
  a
);

parameter SIZE = 4;

input [(SIZE-1):0] a;

wire [(SIZE-1):0] a, b;

assign b = ~a;

endmodule
