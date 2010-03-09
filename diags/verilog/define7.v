/*
 Name:        define7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/28/2010
 Purpose:     
*/

`define DELAY

module main;

reg a;

initial begin
	`DELAY
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "define7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
