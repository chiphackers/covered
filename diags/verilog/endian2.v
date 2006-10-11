module main;

parameter FOO = 3;

reg [0:FOO] a;
reg [FOO:6] b;

initial begin
	a = {FOO{1'b0}};
        b = {FOO{1'b0}};
	#5;
	a = 'hc;
	b = a[1:FOO];
end

initial begin
`ifndef VPI
        $dumpfile( "endian2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
