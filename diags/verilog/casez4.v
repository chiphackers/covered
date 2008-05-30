/*
 Name:        casez4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [39:0] a;
reg        b; 

always @*
  casez( a )
    40'h0000000000 :  b = 1'b0;
    40'h0000000001 :  b = 1'b1;
  endcase

initial begin
`ifdef DUMP
        $dumpfile( "casez4.vcd" );
        $dumpvars( 0, main );
`endif
	a = 40'h0;
	#5;
	a = 40'h1;
        #5;
        $finish;
end

endmodule
