module main;

foo #(8) bar();

initial begin
        $dumpfile( "generate1.vcd" );
        $dumpvars( 0, main );
	bar. \U[0] .a = 2'h0;
	bar. \U[1] .a = 2'h1;
	bar. \U[2] .a = 2'h2;
	bar. \U[3] .a = 2'h3;
        #10;
        $finish;
end

endmodule

module foo;

parameter max = 4;

generate
  genvar i;
  for( i=0; i<max; i=i+1 )
    begin : U
      reg [1:0] a;
    end
endgenerate

endmodule
