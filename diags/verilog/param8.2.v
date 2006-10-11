module main;

reg       a;
reg [1:0] b;
reg [2:0] c;

foo bar( a, b, c );

initial begin
`ifndef VPI
        $dumpfile( "param8.2.vcd" );
        $dumpvars( 0, main );
`endif
	a = 1'h0;
	b = 2'h0;
        c = 3'h0;
        #10;
        $finish;
end

endmodule


module foo #(parameter [1:0] SIZE1=1, SIZE2=6, parameter FOO=3) (
  input wire [(SIZE1-1):0] a,
  input wire [(SIZE2-1):0] b,
  input wire [(FOO-1):0]   c
);

endmodule
