module main;

generate
  genvar i;
  for( i=0; i<4; i=i+1 )
    begin : U
      reg [1:0] a;
    end 
endgenerate

initial begin
        $dumpfile( "generate1.vcd" );
        $dumpvars( 0, main );
	\U[0] .a = 2'h0;
	\U[1] .a = 2'h1;
	\U[2] .a = 2'h2;
	\U[3] .a = 2'h3;
        #10;
        $finish;
end

endmodule
