/*
 Name:        mem4.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that multi-bit indexing into memories outputs properly to the report file.
*/

module main;

reg [1:0] a;
reg [2:0] mem[7:0];
integer   i;

initial begin
	a = 2'b00;
	for( i=0; i<8; i=i+1 )
          #1 mem[i] = i;
	a = mem[2][2:1] & mem[6][1+:2] & mem[7][2-:2];
end

initial begin
`ifdef DUMP
        $dumpfile( "mem4.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
