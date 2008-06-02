/*
 Name:        param17.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/31/2008
 Purpose:     Verify that parameter sized port that is also declared works properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [3:0] a;

foo f( a );

initial begin
`ifdef DUMP
        $dumpfile( "param17.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'b0;
        #10;
        $finish;
end

endmodule


module foo(
  a
);

parameter SIZE = 4;

input [(SIZE-1):0] a;

wire [(SIZE-1):0] a;

endmodule
