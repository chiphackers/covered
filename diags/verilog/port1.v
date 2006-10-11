module main;

reg        i1;
reg  [1:0] i2;
wire       o1, io1;
wire [1:0] o2, io2;

foo bar (
  .i1  ( i1  ),
  .i2  ( i2  ),
  .o1  ( o1  ),
  .o2  ( o2  ),
  .io1 ( io1 ),
  .io2 ( io2 )
);

initial begin
`ifndef VPI
        $dumpfile( "port1.vcd" );
        $dumpvars( 0, main );
`endif
	i1 = 1'b0;
	i2 = 2'b10;
        #10;
        $finish;
end

endmodule


module foo (
  input  wire       i1,
  input  wire [1:0] i2,
  output wire       o1,
  output reg  [1:0] o2,
  inout  wire       io1,
  inout  wire [1:0] io2
);

assign o1 = i1;

always @* o2 = i2;

endmodule
