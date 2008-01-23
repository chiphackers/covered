module main;

generate
  genvar i;
  for( i=0; i<4; i=i+2 )
    begin : U
     reg [1:0] a;
     initial begin : V
             a = 2'b0;
             #10;
             a = i;
     end
    end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate6.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
