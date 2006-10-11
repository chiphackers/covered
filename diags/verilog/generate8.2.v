module main;

generate
  genvar i, j;
  for( i=0; i<2; i=i+1 )
    begin : U
     for( j=0; j<4; j=j+1 )
       begin : V
        reg [3:0] x = 0;
        initial begin
                x = (i * 4) + j;
        end
       end
    end
endgenerate

initial begin
`ifndef VPI
        $dumpfile( "generate8.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
