/*
 Name:        null_stmt1.9.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/13/2008
 Purpose:     Verify that null statements after default statements work properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	case( a )
          1'b1 :  b = 1'b1;
          default : ;
        endcase
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "null_stmt1.9.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
