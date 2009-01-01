/*
 Name:        mem4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/10/2008
 Purpose:     Verifies that memories can be output to report files in combinational logic.
*/

module main;

reg       a;
reg [1:0] mem[3:0];
integer   i;

initial begin
	a = 1'b0;
	for( i=0; i<4; i=i+1 )
          #1 mem[i] = i;
	$display( "mem[1][0]: %d, mem[2][0]: %d", mem[1][0], mem[2][0] );
	a = mem[1][0] | mem[2][0];
end

initial begin
`ifdef DUMP
        $dumpfile( "mem4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
