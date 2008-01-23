module main;

reg [0:31] a;
reg        b;
reg [31:0] c;
reg [0:3]  d, e;
reg [0:31] f;

initial begin
	a = 32'h0;
	b = 1'b0;
	c = 32'h0;
        d = 4'h0;
	e = 4'h0;
	f = 32'h0;
	#5;
	a = 32'h01234567;
	c = a;
	b = a[0];
        d = a[4:7];
	e = c[7:4];
	f[0]   = c[0];
	f[1:3] = c[3:1];
	{f[7],f[4:6]} = c[7:4];
end

initial begin
`ifdef DUMP
        $dumpfile( "endian1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
