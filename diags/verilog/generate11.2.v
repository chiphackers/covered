module main;

genvar i;

generate
  for( i=0; i<4; i=i+1 )
    begin : U
      reg [1:0] x;
    end
  for( i=0; i<4; i=i+1 )
    begin : V
      initial begin
              U[0].x = 2'b0;
              U[0].x = i;
      end
    end
endgenerate

initial begin
        $dumpfile( "generate11.2.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
