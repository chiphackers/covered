module main;

parameter max_a_size = 8;

generate
  genvar i;
  for( i=0; i<4; i=i+1 )
    begin : U
      reg [max_a_size-i:0] a;
    end
endgenerate

initial begin
        $dumpfile( "generate1.1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
