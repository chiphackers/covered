/*
 Name:        mem5.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/02/2009
 Purpose:     Verifies that a memory accessed by an unpacked dimension reports
              the correct memory coverage (bits in last dimension should be reported
              as written/read).
*/

module main;

reg [3:0] mem[3:0][3:0];
reg [3:0] store[3:0][3:0];

initial begin
	store[1][0] = 4'hf;
	store[1][1] = 4'hf;
	store[1][2] = 4'hf;
	store[1][3] = 4'hf;
	#5;
        mem[1]      = store[1];
end

initial begin
`ifdef DUMP
        $dumpfile( "mem5.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
