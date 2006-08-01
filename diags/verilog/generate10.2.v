module main;

generate
  if( 0 < 1 )
    begin
     initial begin
             $dumpfile( "generate10.2.vcd" );
             $dumpvars( 0, main );
             #30;
             $finish;
     end
     reg a;
     reg b;
     reg c;
     initial begin
             a = 1'b0;
             b = 1'b1;
             c = 1'b0;
             #10;
             a = b | c;
             #10;
             a = b & c;
     end
    end
endgenerate

endmodule
