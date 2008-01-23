module main;

reg  [1:0]  a[3:0];
reg  [2:0]  b[3:0], c[2:0];

integer i;

always @(b[c[0]])
  a[2] <= b[c[0]];

initial begin
`ifdef DUMP
	$dumpfile( "mem1.vcd" );
	$dumpvars( 0, main );
`endif
        for( i=0; i<8; i=i+1 )
          b[i] = i;
	c[0] = 3'b001;
	#20;
	$finish;
end

endmodule
