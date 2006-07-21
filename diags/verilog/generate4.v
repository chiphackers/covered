module main;

genvar i;

generate
  for( i=0; i<4; i=i+1 )
    begin : U
     foo bar();
    end 
endgenerate

initial begin
        $dumpfile( "generate4.vcd" );
        $dumpvars( 0, main );
        #20;
        $finish;
end

initial begin
	U[3].bar.x = 1'b0;
	#10;
	U[3].bar.x = 1'b1;
end

endmodule

//-----------------------------

module foo;

reg x;

endmodule
