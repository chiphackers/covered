/*
 Name:        err5.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/23/2008
 Purpose:     Verify that a vector_db_read error can be handled properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
`ifdef DUMP
        $dumpfile( "err5.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
