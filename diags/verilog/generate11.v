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
              U[i].x = 2'b0;
              U[i].x = i;
      end
    end
endgenerate

initial begin
`ifndef VPI
        $dumpfile( "generate11.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
