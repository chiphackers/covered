/*
 Name:        merge_err1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/17/2008
 Purpose:     
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

initial begin
`ifdef DUMP
        $dumpfile( "merge_err1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
