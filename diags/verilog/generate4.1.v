module main;

genvar i;

generate
  for( i=0; i<2; i=i+1 )
    begin : U
     foo bar[3:0]();
    end 
endgenerate

initial begin
`ifndef VPI
        $dumpfile( "generate4.1.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

initial begin
	U[1].bar[2].x = 1'b0;
	#10;
	U[1].bar[2].x = 1'b1;
end

endmodule

//-----------------------------

module foo;

reg x;

endmodule
