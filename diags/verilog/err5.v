/*
 Name:        err5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/23/2008
 Purpose:     Used to verify the user error reporting capability of the vector_db_read function.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a;

initial begin
`ifdef DUMP
        $dumpfile( "err5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
