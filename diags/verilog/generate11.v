module main;

genvar i;

generate
  for( i=0; i<4; i=i+1 )
    begin : U
      reg x;
    end
  for( i=0; i<4; i=i+1 )
    begin : V
      initial U[i].x = i;
    end
endgenerate

initial begin
        $dumpfile( "generate11.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
