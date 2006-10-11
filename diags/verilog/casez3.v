module main;

wire      a;
real      mem0, mem1;
reg [1:0] entry;

always @*
  casez( a )
    1'b0    :  entry = mem0;
    1'b1    :  entry = mem1;
    default :  entry = 2'bx;
  endcase

initial begin
`ifndef VPI
        $dumpfile( "casez3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
