module main;

foo #(8) bar();

initial begin
`ifndef VPI
        $dumpfile( "generate1.vcd" );
        $dumpvars( 0, main );
`endif
	#20;
	$finish;
end

initial begin
	bar. \U[0] .a = 2'h0;
	bar. \U[1] .a = 2'h0;
	bar. \U[2] .a = 2'h0;
	bar. \U[3] .a = 2'h0;
        #10;
	bar. \U[0] .a = 2'h0;
	bar. \U[1] .a = 2'h1;
	bar. \U[2] .a = 2'h2;
	bar. \U[3] .a = 2'h3;
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
