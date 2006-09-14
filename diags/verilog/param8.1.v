module main;

reg [2:0] a;

foo #(3) bar (
  .a( a )
);

initial begin
        $dumpfile( "param8.1.vcd" );
        $dumpvars( 0, main );
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
