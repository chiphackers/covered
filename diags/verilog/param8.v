module main;

reg [3:0] a;

foo bar (
  .a( a )
);

initial begin
`ifndef VPI
        $dumpfile( "param8.vcd" );
        $dumpvars( 0, main );
`endif
	a = 4'h0;
        #10;
        $finish;
end

endmodule


module foo #(parameter SIZE=4) (
  input wire [(SIZE-1):0] a
);

parameter ZERO = 0;

wire [(SIZE-1):0] b;

assign b = a | ZERO;

endmodule
