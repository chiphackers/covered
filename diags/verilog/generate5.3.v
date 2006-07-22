module main;

localparam init = 1;

reg a;

generate
  if( init == 0 )
    initial begin
            a = 1'b0;
            #10;
            a = 1'b1;
    end
  else
    initial begin
            a = 1'b1;
            #10;
            a = 1'b0;
    end
endgenerate

initial begin
        $dumpfile( "generate5.3.vcd" );
        $dumpvars( 0, main );
        #20;
        $finish;
end

endmodule
