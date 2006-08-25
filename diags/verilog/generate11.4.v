module main;

generate
  genvar i, j;
  for( i=0; i<4; i++ )
    begin : U
     reg [1:0] x = 0;
    end
endgenerate

initial #5 U[00].x = 1'b1;

initial begin
        $dumpfile( "generate11.4.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
