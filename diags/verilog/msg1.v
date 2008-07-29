/*
 Name:        msg1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        07/02/2008
 Purpose:     Makes use of the -m option to the score command
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "msg1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
