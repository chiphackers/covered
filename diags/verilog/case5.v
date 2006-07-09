module main;

wire      a;
reg [1:0] mem[0:1];
reg [1:0] entry;

always @*
  case( a )
    1'b0    :  entry = mem[0];
    1'b1    :  entry = mem[1];
    default :  entry = 2'bx;
  endcase

initial begin
        $dumpfile( "case5.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
